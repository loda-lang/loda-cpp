#include "generator_v4.hpp"
#include "generator_v1.hpp"
#include "generator.hpp"

#include "parser.hpp"
#include "program_util.hpp"
#include "util.hpp"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>

std::string getGenV4Home()
{
  // no trailing / here
  return getLodaHome() + "gen_v4";
}

std::string getNumFilesPath()
{
  return getGenV4Home() + "/numfiles.txt";
}

ProgramState::ProgramState()
    : index( 0 ),
      generated( 0 )
{
}

void ProgramState::validate() const
{
  if ( index < 1 || index >= 10000 )
  {
    throw std::runtime_error( "invalid program state index: " + std::to_string( index ) );
  }
}

std::string ProgramState::getPath() const
{
  std::stringstream s;
  s << getGenV4Home() << "/S" << std::setw( 4 ) << std::setfill( '0' ) << index << ".txt";
  return s.str();
}

void ProgramState::load()
{
  validate();
  Parser parser;
  Program p = parser.parse( getPath() );
  size_t step = 0;
  start.ops.clear();
  current.ops.clear();
  end.ops.clear();
  for ( auto& op : p.ops )
  {
    if ( op.type == Operation::Type::NOP && !op.comment.empty() )
    {
      if ( op.comment == "start" )
      {
        step = 1;
      }
      else if ( op.comment.rfind( "current: ", 0 ) == 0 )
      {
        step = 2;
        auto sub = op.comment.substr( 9 );
        generated = stoll( sub );
      }
      else if ( op.comment == "end" )
      {
        step = 3;
      }
      else
      {
        std::runtime_error( "program state load error" );
      }
      continue;
    }
    switch ( step )
    {
    case 1:
      start.ops.push_back( op );
      break;
    case 2:
      current.ops.push_back( op );
      break;
    case 3:
      end.ops.push_back( op );
      break;
    default:
      std::runtime_error( "program state load error" );
    }
  }
}

void ProgramState::save() const
{
  validate();
  Program p;
  Operation nop( Operation::Type::NOP );
  nop.comment = "start";
  p.ops.push_back( nop );
  p.ops.insert( p.ops.end(), start.ops.begin(), start.ops.end() );
  nop.comment = "current: " + std::to_string( generated );
  p.ops.push_back( nop );
  p.ops.insert( p.ops.end(), current.ops.begin(), current.ops.end() );
  nop.comment = "end";
  p.ops.push_back( nop );
  p.ops.insert( p.ops.end(), end.ops.begin(), end.ops.end() );
  std::ofstream f( getPath() );
  ProgramUtil::print( p, f );
  f.close();
}

GeneratorV4::GeneratorV4( const Config &config, const Stats &stats, int64_t seed )
    : Generator( config, stats, seed )
{
  // obtain lock
  FolderLock lock( getGenV4Home() );
  std::ifstream nf( getNumFilesPath() );
  if ( !nf.good() )
  {
    init( stats, seed );
  }
  nf.close();
  load();
}

void GeneratorV4::init( const Stats &stats, int64_t seed )
{
  Log::get().info( "Initializing state of generator v4 in " + getGenV4Home() );

  Generator::Config config;
  config.version = 1;
  config.replicas = 1;
  config.loops = true;
  config.calls = false;
  config.indirect_access = false;

  std::vector<Program> programs;
  for ( int64_t length = 3; length <= 20; length++ )
  {
    int64_t count = pow( 1.25, length );
    config.length = length;
    config.max_constant = std::min<int64_t>( length / 4, 2 );
    config.max_index = std::min<int64_t>( length / 4, 2 );
    GeneratorV1 gen_v1( config, stats, seed );
    for ( int64_t c = 0; c < count; c++ )
    {
      programs.push_back( gen_v1.generateProgram() );
    }
  }

  std::sort( programs.begin(), programs.end() );

  ensureDir( getGenV4Home() );

  ProgramState s;
  s.index = 1;
  s.generated = 0;
  s.start.push_back( Operation::Type::MOV, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::CONSTANT, 0 );
  for ( auto& p : programs )
  {
    if ( p == s.start )
    {
      continue;
    }
    s.current = s.start;
    s.end = p;
    s.save();
    s.start = p;
    s.index++;
  }

  std::ofstream nf( getNumFilesPath() );
  nf << (s.index - 1) << std::endl;
  nf.close();
}

void GeneratorV4::load()
{
  std::ifstream nf( getNumFilesPath() );
  if ( !nf.good() )
  {
    Log::get().error( "File not found: " + getNumFilesPath(), true );
  }
  int64_t num_files;
  nf >> num_files;
  nf.close();
  if ( num_files < 1 || num_files >= 10000 )
  {
    Log::get().error( "Invalid number of files: " + std::to_string( num_files ), true );
  }
  int64_t attempts = num_files * 100;
  do
  {
    state = ProgramState();
    state.index = (gen() % num_files) + 1;
    state.load();
    iterator = Iterator( state.current );
  } while ( state.end < state.current && --attempts );
  if ( !attempts )
  {
    Log::get().error( "Looks like we already generated all programs!", true );
  }
  Log::get().info(
      "Working on gen_v4 block " + std::to_string( state.index ) + " (" + std::to_string( state.generated )
          + " generated programs)" );
}

Program GeneratorV4::generateProgram()
{
  state.current = iterator.next();
  state.generated++;
  if ( state.generated % 10000 == 0 )
  {
    FolderLock lock( getGenV4Home() );
    state.save();
    load();
  }
  return state.current;
}

std::pair<Operation, double> GeneratorV4::generateOperation()
{
  throw std::runtime_error( "unsupported operation in generator v4" );
  return
  {};
}
