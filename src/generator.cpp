#include "generator.hpp"

#include "generator_v1.hpp"
#include "generator_v2.hpp"
#include "generator_v3.hpp"
#include "util.hpp"
#include "jute.h"

Generator::UPtr Generator::Factory::createGenerator( const Config &config, int64_t seed )
{
  Generator::UPtr generator;
  switch ( config.version )
  {
  case 1:
  {
    generator.reset( new GeneratorV1( config, seed ) );
    break;
  }
  case 2:
  {
    generator.reset( new GeneratorV2( config, seed ) );
    break;
  }
  case 3:
  {
    generator.reset( new GeneratorV3( config, seed ) );
    break;
  }
  default:
  {
    Log::get().error( "Invalid generator version: " + std::to_string( config.version ), true );
    break;
  }
  }
  return generator;
}

int64_t get_jint( jute::jValue &v, const std::string &key, int64_t def )
{
  if ( v[key].get_type() == jute::jType::JNUMBER )
  {
    return v[key].as_int();
  }
  return def;
}

bool get_jbool( jute::jValue &v, const std::string &key, bool def )
{
  if ( v[key].get_type() == jute::jType::JBOOLEAN )
  {
    return v[key].as_bool();
  }
  return def;
}

std::vector<Generator::Config> Generator::Config::load( std::istream &in )
{
  std::vector<Generator::Config> configs;
  std::string str = "";
  std::string tmp;
  while ( getline( in, tmp ) )
  {
    str += tmp;
  }
  auto spec = jute::parser::parse( str );
  auto gens = spec["generators"];
  for ( int i = 0; i < gens.size(); i++ )
  {
    auto g = gens[i];
    Generator::Config c;
    c.version = get_jint( g, "version", 1 );
    c.replicas = get_jint( g, "replicas", 1 );
    c.length = get_jint( g, "length", 20 );
    c.max_constant = get_jint( g, "maxConstant", 4 );
    c.max_index = get_jint( g, "maxIndex", 4 );
    c.loops = get_jbool( g, "loops", true );
    c.indirect_access = get_jbool( g, "indirectAccess", false );
    switch ( g["template"].get_type() )
    {
    case jute::jType::JSTRING:
    {
      c.program_template = g["template"].as_string();
      configs.push_back( c );
      break;
    }
    case jute::jType::JARRAY:
    {
      auto a = g["template"];
      for ( int i = 0; i < a.size(); i++ )
      {
        if ( a[i].get_type() == jute::jType::JSTRING )
        {
          c.program_template = a[i].as_string();
          configs.push_back( c );
        }
      }
      break;
    }
    default:
    {
      configs.push_back( c );
      break;
    }
    }
  }
  return configs;
}

Generator::Generator( const Config &config, int64_t seed )
    :
    config( config )
{
  gen.seed( seed );

  metric_labels = { { "generator_version", std::to_string( config.version ) }, { "num_operations", std::to_string(
      config.length ) }, { "max_constant", std::to_string( config.max_constant ) }, { "max_index", std::to_string(
      config.max_index ) }, { "loops", std::to_string( config.loops ) }, { "indirect_access", std::to_string(
      config.indirect_access ) }, };
  if ( !config.program_template.empty() )
  {
    metric_labels.emplace( "program_template", config.program_template );
  }
}

void Generator::generateStateless( Program &p, size_t num_operations )
{
  // fill program with random operations
  size_t nops = 0;
  while ( p.ops.size() + nops < num_operations )
  {
    auto next_op = generateOperation();
    if ( next_op.first.type == Operation::Type::NOP || next_op.first.type == Operation::Type::LPE )
    {
      nops++;
      continue;
    }
    size_t position = (next_op.second * (p.ops.size() + 1));
    p.ops.emplace( p.ops.begin() + position, Operation( next_op.first ) );
    if ( next_op.first.type == Operation::Type::LPB )
    {
      position = ((position + p.ops.size()) / 2) + 1;
      p.ops.emplace( p.ops.begin() + position, Operation( Operation::Type::LPE ) );
    }
  }
}

void Generator::applyPostprocessing( Program &p )
{
  auto written_cells = fixCausality( p );
  ensureSourceNotOverwritten( p );
  ensureTargetWritten( p, written_cells );
  ensureMeaningfulLoops( p );
}

std::vector<number_t> Generator::fixCausality( Program &p )
{
  // fix causality of read operations
  std::vector<number_t> written_cells;
  written_cells.push_back( 0 );
  int64_t new_cell;
  for ( size_t position = 0; position < p.ops.size(); position++ )
  {
    auto &op = p.ops[position];
    auto &meta = Operation::Metadata::get( op.type );

    // fix source operand in new operation
    if ( meta.num_operands == 2 && op.source.type == Operand::Type::DIRECT
        && std::find( written_cells.begin(), written_cells.end(), op.source.value ) == written_cells.end() )
    {
      new_cell = op.source.value % written_cells.size(); // size of written_cells is >=1
      if ( new_cell == op.target.value )
      {
        new_cell = (new_cell + 1) % written_cells.size();
      }
      op.source.value = written_cells[new_cell];
    }

    // fix target operand in new operation
    if ( meta.num_operands > 0 && meta.is_reading_target && op.type != Operation::Type::ADD
        && op.target.type == Operand::Type::DIRECT
        && std::find( written_cells.begin(), written_cells.end(), op.target.value ) == written_cells.end() )
    {
      new_cell = op.target.value % written_cells.size();
      if ( new_cell == op.source.value )
      {
        new_cell = (new_cell + 1) % written_cells.size();
      }
      op.target.value = written_cells[new_cell];
    }

    // check if target cell not written yet
    if ( meta.is_writing_target && op.target.type == Operand::Type::DIRECT
        && std::find( written_cells.begin(), written_cells.end(), op.target.value ) == written_cells.end() )
    {
      // update written cells
      written_cells.push_back( op.target.value );
    }
  }
  return written_cells;
}

void Generator::ensureSourceNotOverwritten( Program &p )
{
  // make sure that the initial value does not get overridden immediately
  bool resets;
  for ( auto &op : p.ops )
  {
    if ( op.target.type == Operand::Type::DIRECT && op.target.value == Program::INPUT_CELL )
    {
      resets = false;
      if ( op.type == Operation::Type::MOV || op.type == Operation::Type::CLR )
      {
        resets = true;
      }
      else if ( (op.source == op.target)
          && (op.type == Operation::Type::SUB || op.type == Operation::Type::TRN || op.type == Operation::Type::DIV
              || op.type == Operation::Type::MOD) )
      {
        resets = true;
      }
      if ( resets )
      {
        op.target.value = (gen() % 4) + 1;
      }
    }
    else if ( op.source.type == Operand::Type::DIRECT && op.source.value == Program::INPUT_CELL )
    {
      break;
    }
  }
}

void Generator::ensureTargetWritten( Program &p, const std::vector<number_t> &written_cells )
{
  // make sure that the target value gets written
  bool written = false;
  for ( auto &op : p.ops )
  {
    if ( op.type != Operation::Type::LPB && Operation::Metadata::get( op.type ).num_operands == 2
        && op.target.type == Operand::Type::DIRECT && op.target.value == Program::OUTPUT_CELL )
    {
      written = true;
      break;
    }
  }
  if ( !written )
  {
    number_t source = Program::INPUT_CELL;
    if ( !written_cells.empty() )
    {
      source = written_cells.at( gen() % written_cells.size() );
    }
    p.ops.push_back(
        Operation( Operation::Type::MOV, Operand( Operand::Type::DIRECT, Program::OUTPUT_CELL ),
            Operand( Operand::Type::DIRECT, source ) ) );
  }
}

void Generator::ensureMeaningfulLoops( Program &p )
{
  // make sure loops do something
  number_t mem = 0;
  number_t num_ops = 0;
  bool can_descent = false;
  for ( size_t i = 0; i < p.ops.size(); i++ )
  {
    switch ( p.ops[i].type )
    {
    case Operation::Type::LPB:
    {
      mem = p.ops[i].target.value;
      can_descent = false;
      num_ops = 0;
      break;
    }
    case Operation::Type::ADD:
    case Operation::Type::MUL:
    case Operation::Type::POW:
    case Operation::Type::FAC:
      num_ops++;
      break;
    case Operation::Type::SUB:
    case Operation::Type::LOG:
    case Operation::Type::MOV:
    case Operation::Type::DIV:
    case Operation::Type::MOD:
    case Operation::Type::GCD:
    case Operation::Type::BIN:
    case Operation::Type::CMP:
      num_ops++;
      if ( p.ops[i].target.value == mem )
      {
        can_descent = true;
      }
      break;
    case Operation::Type::LPE:
    {
      if ( !can_descent )
      {
        Operation dec;
        dec.target = Operand( Operand::Type::DIRECT, mem );
        dec.source = Operand( Operand::Type::CONSTANT, (gen() % 9) + 1 );
        switch ( gen() % 3 )
        {
        case 0:
          dec.type = Operation::Type::SUB;
          break;
        case 1:
          dec.type = Operation::Type::DIV;
          dec.source.value++;
          break;
        case 2:
          dec.type = Operation::Type::MOD;
          dec.source.value++;
          break;
        }
        p.ops.insert( p.ops.begin() + i, dec );
        i++;
      }
      if ( num_ops < 2 )
      {
        for ( int64_t j = (gen() % 3) + 3; j > 0; j-- )
        {
          auto op = generateOperation();
          if ( op.first.type != Operation::Type::LPB && op.first.type != Operation::Type::LPE )
          {
            p.ops.insert( p.ops.begin() + i, op.first );
            i++;
          }
        }
      }
      break;
    }
    default:
      break;
    }
  }
}

MultiGenerator::MultiGenerator( const Settings &settings, int64_t seed )
{
  std::mt19937 gen( seed * 13 + 17 );
  std::ifstream loda_conf( settings.loda_config );
  configs = Generator::Config::load( loda_conf );
  if ( configs.empty() )
  {
    Log::get().error( "No generators configurations found", true );
  }
  generators.resize( configs.size() );
  for ( size_t i = 0; i < configs.size(); i++ )
  {
    generators[i] = Generator::Factory::createGenerator( configs[i], gen() );
  }
  generator_index = gen() % configs.size();
  replica_index = gen() % configs.at( gen() % configs.size() ).replicas;
}

Generator* MultiGenerator::getGenerator()
{
  return generators[generator_index].get();
}

void MultiGenerator::next()
{
  if ( replica_index >= configs[generator_index].replicas )
  {
    generator_index = (generator_index + 1) % configs.size();
    replica_index = 0;
  }
  else
  {
    replica_index++;
  }
}
