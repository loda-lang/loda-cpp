#include "test.hpp"

#include "config.hpp"
#include "generator_v1.hpp"
#include "interpreter.hpp"
#include "iterator.hpp"
#include "matcher.hpp"
#include "miner.hpp"
#include "minimizer.hpp"
#include "number.hpp"
#include "oeis_manager.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "semantics.hpp"
#include "stats.hpp"

#include <deque>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <stdexcept>

Test::Test()
    : manager( settings )
{
}

void Test::all()
{
  // fast tests
  semantics();
  memory();
  config();
  steps();
  collatz();
  linearMatcher();
  deltaMatcher();
  optimizer();
  knownPrograms();

  // slow tests
  size_t tests = 100;
  ackermann();
  stats();
  oeisSeq();
  iterator( tests );
  minimizer( tests );
}

number_t read_num( const std::string &s )
{
  return (s == "NI") ? NUM_INF : std::stoll( s );
}

void check_inf( number_t n )
{
  if ( n != NUM_INF )
  {
    Log::get().error( "Expected infinity instead of " + std::to_string( n ), true );
  }
}

void Test::semantics()
{
  for ( auto &type : Operation::Types )
  {
    if ( type == Operation::Type::NOP || type == Operation::Type::LPB || type == Operation::Type::LPE
        || type == Operation::Type::CLR || type == Operation::Type::CAL || type == Operation::Type::DBG )
    {
      continue;
    }
    auto &meta = Operation::Metadata::get( type );
    std::string test_path = "tests/semantics/" + Operation::Metadata::get( type ).name + ".csv";
    std::ifstream test_file( test_path );
    if ( !test_file.good() )
    {
      Log::get().error( "Test file not found: " + test_path, true );
      continue;
    }
    Log::get().info( "Testing " + test_path );
    std::string line, s, t, r;
    number_t op1 = 0, op2 = 0, expected_op, result_op;
    std::getline( test_file, line ); // skip header
    while ( std::getline( test_file, line ) )
    {
      if ( line.empty() || line[0] == '#' )
      {
        continue;
      }
      std::stringstream ss( line );
      std::getline( ss, s, ',' );
      if ( meta.num_operands == 2 )
      {
        std::getline( ss, t, ',' );
      }
      std::getline( ss, r );
      op1 = read_num( s );
      if ( meta.num_operands == 2 )
      {
        op2 = read_num( t );
      }
      expected_op = read_num( r );
      result_op = Interpreter::calc( type, op1, op2 );
      if ( result_op != expected_op )
      {
        Log::get().error(
            "Unexpected value for " + meta.name + "(" + std::to_string( op1 ) + "," + std::to_string( op2 )
                + "); expected " + std::to_string( expected_op ) + "; got " + std::to_string( result_op ), true );
      }
    }
    if ( type != Operation::Type::MOV )
    {
      check_inf( Interpreter::calc( type, NUM_INF, 0 ) );
      check_inf( Interpreter::calc( type, NUM_INF, 1 ) );
      check_inf( Interpreter::calc( type, NUM_INF, -1 ) );
    }
    if ( meta.num_operands == 2 )
    {
      check_inf( Interpreter::calc( type, 0, NUM_INF ) );
      check_inf( Interpreter::calc( type, 1, NUM_INF ) );
      check_inf( Interpreter::calc( type, -1, NUM_INF ) );
    }
  }
}

void checkMemory( const Memory &mem, number_t index, number_t value )
{
  if ( mem.get( index ) != value )
  {
    Log::get().error(
        "Unexpected memory value at index " + std::to_string( index ) + "; expected: " + std::to_string( value )
            + "; found: " + std::to_string( mem.get( index ) ), true );
  }
}

void Test::memory()
{
  Log::get().info( "Testing memory" );

  // get and set
  Memory base;
  number_t size = 100;
  for ( number_t i = 0; i < size; i++ )
  {
    base.set( i, i );
    checkMemory( base, i, i );
  }
  checkMemory( base, -1, 0 );
  checkMemory( base, size + 1, 0 );

  // fragments
  size_t max_frag_length = 50;
  for ( number_t start = -10; start < size + 10; start++ )
  {
    for ( size_t length = 0; length < max_frag_length; length++ )
    {
      auto frag = base.fragment( start, length, true );
      for ( number_t i = 0; i < (number_t) length; i++ )
      {
        number_t j = start + i; // old index
        number_t v = (j < 0 || j >= size) ? 0 : j;
        checkMemory( frag, i, v );
      }
      checkMemory( frag, -1, 0 );
      checkMemory( frag, -2, 0 );
      checkMemory( frag, length, 0 );
      checkMemory( frag, length + 1, 0 );
    }
  }
}

void validateIterated( const Program& p )
{
  ProgramUtil::validate( p );
  if ( ProgramUtil::numOps( p, Operand::Type::INDIRECT ) > 0 )
  {
    throw std::runtime_error( "Iterator generated indirect memory access" );
  }
  for ( size_t i = 0; i < p.ops.size(); i++ )
  {
    auto& op = p.ops[i];
    if ( op.type == Operation::Type::LPB && (op.source.type != Operand::Type::CONSTANT || op.source.value <= 0) )
    {
      throw std::runtime_error( "Iterator generated wrong loop" );
    }
    if ( op.type == Operation::Type::CLR || op.type == Operation::Type::CAL || op.type == Operation::Type::LOG
        || op.type == Operation::Type::MIN || op.type == Operation::Type::MAX )
    {
      throw std::runtime_error( "Unsupported operation type" );
    }
  }
  for ( size_t i = 1; i < p.ops.size(); i++ )
  {
    if ( p.ops[i - 1].type == Operation::Type::LPB && p.ops[i].type == Operation::Type::LPE )
    {
      throw std::runtime_error( "Iterator generated empty loop" );
    }
  }
}

void Test::iterator( size_t tests )
{
  const int64_t count = 100000;
  std::random_device rand;
  for ( size_t test = 0; test < tests; test++ )
  {
    if ( test % 10 == 0 )
    {
      Log::get().info( "Testing iterator " + std::to_string( test ) );
    }

    // generate a random start program
    Generator::Config config;
    config.version = 1;
    config.loops = true;
    config.calls = false;
    config.indirect_access = false;

    config.length = std::max<int64_t>( test / 4, 2 );
    config.max_constant = std::max<int64_t>( test / 4, 2 );
    config.max_index = std::max<int64_t>( test / 4, 2 );
    GeneratorV1 gen_v1( config, manager.getStats(), rand() );
    Program start, p, q;
    while ( true )
    {
      try
      {
        start = gen_v1.generateProgram();
        validateIterated( start );
        break;
      }
      catch ( const std::exception& )
      {
        //ignore
      }
    }
    // iterate and check
    Iterator it( start );
    for ( int64_t i = 0; i < count; i++ )
    {
      p = it.next();
      try
      {
        validateIterated( p );
        if ( i > 0 && (p < q || !(q < p) || p == q) )
        {
          throw std::runtime_error( "Iterator violates program order" );
        }
      }
      catch ( std::exception& e )
      {
        ProgramUtil::print( q, std::cerr );
        std::cerr << std::endl;
        ProgramUtil::print( p, std::cerr );
        Log::get().error( e.what(), true );
      }
      q = p;
    }
    if ( it.getSkipped() > 0.01 * count )
    {
      Log::get().error( "Too many skipped invalid programs: " + std::to_string( it.getSkipped() ), true );
    }
  }
}

void Test::knownPrograms()
{
  testSeq( 5, Sequence( { 1, 2, 2, 3, 2, 4, 2, 4, 3, 4, 2, 6, 2, 4, 4, 5, 2, 6, 2, 6 } ) );
  testSeq( 30, Sequence( { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 } ) );
  testSeq( 45, Sequence( { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181 } ) );
  testSeq( 79, Sequence( { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536 } ) );
  testSeq( 1489, Sequence( { 0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16, -17 } ) );
}

void Test::steps()
{
  auto file = OeisSequence( 12 ).getProgramPath();
  Log::get().info( "Testing steps for " + file );
  Parser parser;
  Interpreter interpreter( settings );
  auto p = parser.parse( file );
  Memory mem;
  mem.set( Program::INPUT_CELL, 26 );
  auto steps = interpreter.run( p, mem );
  if ( steps != 1 )
  {
    Log::get().error( "unexpected number of steps: " + std::to_string( steps ), true );
  }
}

void checkSeq( const Sequence& s, size_t expected_size, size_t index, number_t expected_value )
{
  if ( s.size() != expected_size )
  {
    Log::get().error(
        "Unexpected number of terms: " + std::to_string( s.size() ) + " (expected " + std::to_string( expected_size )
            + ")", true );
  }
  if ( s[index] != expected_value )
  {
    Log::get().error( "Unexpected terms: " + s.to_string(), true );
  }
}

void Test::oeisSeq()
{
  OeisSequence s( 6 );
  std::remove( s.getBFilePath().c_str() );
  checkSeq( s.getTerms( 20 ), 20, 18, 8 ); // this should fetch the b-file
  checkSeq( s.getTerms( 250 ), 250, 235, 38 );
  checkSeq( s.getTerms( 2000 ), 2000, 1240, 100 );
  checkSeq( s.getTerms( 10000 ), 10000, 9840, 320 );
  checkSeq( s.getTerms( 100000 ), 10000, 9840, 320 ); // only 10000 terms available
  checkSeq( s.getTerms( 10000 ), 10000, 9840, 320 ); // from here on, the b-file should not get re-loaded
  checkSeq( s.getTerms( 2000 ), 2000, 1240, 100 );
  checkSeq( s.getTerms( 250 ), 250, 235, 38 );
  checkSeq( s.getTerms( 20 ), 20, 18, 8 );
}

void Test::ackermann()
{
  std::vector<std::vector<number_t> > values = { { 1, 2, 3, 4, 5 }, { 2, 3, 4, 5, 6 }, { 3, 5, 7, 9, 11 }, { 5, 13, 29,
      61, 125 }, { 13, 65533 } };
  testBinary( "ack", "programs/general/ackermann.asm", values );
}

void Test::collatz()
{
  Log::get().info( "Testing collatz validator using A006577" );
  std::vector<number_t> values = { 0, 1, 7, 2, 5, 8, 16, 3, 19, 6, 14, 9, 9, 17, 17, 4, 12, 20, 20, 7, 7, 15, 15, 10,
      23, 10, 111, 18, 18, 18, 106, 5, 26, 13, 13, 21, 21, 21, 34, 8, 109, 8, 29, 16, 16, 16, 104, 11, 24, 24, 24, 11,
      11, 112, 112, 19, 32, 19, 32, 19, 19, 107, 107, 6, 27, 27, 27, 14, 14, 14, 102, 22 };
  Sequence s( values );

  if ( !Miner::isCollatzValuation( s ) )
  {
    Log::get().error( "A006577 is not a Collatz valuation", true );
  }
}

void check_int( const std::string &s, int64_t expected, int64_t value )
{
  if ( value != expected )
  {
    Log::get().error( "expected " + s + ": " + std::to_string( expected ) + ", got: " + std::to_string( value ), true );
  }
}

void check_str( const std::string &s, const std::string& expected, const std::string& value )
{
  if ( value != expected )
  {
    Log::get().error( "expected " + s + ": " + expected + ", got: " + value, true );
  }
}

void Test::config()
{
  Log::get().info( "Testing config" );

  Settings settings;
  settings.loda_config = "tests/config/test_loda.json";
  settings.miner = "default";
  auto config = ConfigLoader::load( settings );
  check_int( "overwrite", 1, config.overwrite_mode == OverwriteMode::NONE );

  check_int( "generators.size", 3, config.generators.size() );
  check_int( "generators[0].version", 1, config.generators[0].version );
  check_int( "generators[0].length", 30, config.generators[0].length );
  check_int( "generators[0].maxConstant", 3, config.generators[0].max_constant );
  check_int( "generators[0].maxIndex", 4, config.generators[0].max_index );
  check_int( "generators[0].loops", 0, config.generators[0].loops );
  check_int( "generators[0].calls", 1, config.generators[0].calls );
  check_int( "generators[0].indirectAccess", 0, config.generators[0].indirect_access );
  check_str( "generators[0].template", "tmp1", config.generators[0].program_template );
  check_int( "generators[1].version", 1, config.generators[1].version );
  check_int( "generators[1].length", 30, config.generators[1].length );
  check_int( "generators[1].maxConstant", 3, config.generators[1].max_constant );
  check_int( "generators[1].maxIndex", 4, config.generators[1].max_index );
  check_int( "generators[1].loops", 0, config.generators[1].loops );
  check_int( "generators[1].calls", 1, config.generators[1].calls );
  check_int( "generators[1].indirectAccess", 0, config.generators[1].indirect_access );
  check_str( "generators[1].template", "tmp2", config.generators[1].program_template );
  check_int( "generators[2].version", 1, config.generators[2].version );
  check_int( "generators[2].length", 40, config.generators[2].length );
  check_int( "generators[2].maxConstant", 4, config.generators[2].max_constant );
  check_int( "generators[2].maxIndex", 5, config.generators[2].max_index );
  check_int( "generators[2].loops", 1, config.generators[2].loops );
  check_int( "generators[2].calls", 0, config.generators[2].calls );
  check_int( "generators[2].indirectAccess", 1, config.generators[2].indirect_access );
  check_str( "generators[2].template", "", config.generators[2].program_template );

  check_int( "matchers.size", 2, config.matchers.size() );
  check_str( "matchers[0].type", "direct", config.matchers[0].type );
  check_int( "matchers[0].backoff", 1, config.matchers[0].backoff );
  check_str( "matchers[1].type", "linear1", config.matchers[1].type );
  check_int( "matchers[1].backoff", 1, config.matchers[1].backoff );

  settings.miner = "update";
  config = ConfigLoader::load( settings );
  check_int( "overwrite", 1, config.overwrite_mode == OverwriteMode::ALL );

  check_int( "generators.size", 2, config.generators.size() );
  check_int( "generators[0].version", 2, config.generators[0].version );
  check_int( "generators[1].version", 3, config.generators[1].version );

  check_int( "matchers.size", 2, config.matchers.size() );
  check_str( "matchers[0].type", "linear2", config.matchers[0].type );
  check_int( "matchers[0].backoff", 0, config.matchers[0].backoff );
  check_str( "matchers[1].type", "delta", config.matchers[1].type );
  check_int( "matchers[1].backoff", 0, config.matchers[1].backoff );

  settings.miner = "0";
  config = ConfigLoader::load( settings );
  check_int( "generators.size", 3, config.generators.size() );

  settings.miner = "1";
  config = ConfigLoader::load( settings );
  check_int( "generators.size", 2, config.generators.size() );

}

void Test::stats()
{
  Log::get().info( "Testing stats loading and saving" );

  // load stats
  Stats s, t;
  s = manager.getStats();

  // sanity check for loaded stats
  if ( s.num_constants.at( 1 ) == 0 )
  {
    Log::get().error( "Error loading constants counts from stats", true );
  }
  if ( s.num_ops_per_type.at( static_cast<size_t>( Operation::Type::MOV ) ) == 0 )
  {
    Log::get().error( "Error loading operation type counts from stats", true );
  }
  Operation op( Operation::Type::ADD, Operand( Operand::Type::DIRECT, 0 ), Operand( Operand::Type::CONSTANT, 1 ) );
  if ( s.num_operations.at( op ) == 0 )
  {
    Log::get().error( "Error loading operation counts from stats", true );
  }
  if ( s.num_operation_positions.size() < 100000 )
  {
    Log::get().error( "Unexpected number of operation position counts in stats", true );
  }
  OpPos op_pos;
  op_pos.pos = 0;
  op_pos.len = 2;
  op_pos.op.type = Operation::Type::MOV;
  op_pos.op.target = Operand( Operand::Type::DIRECT, 1 );
  op_pos.op.source = Operand( Operand::Type::DIRECT, 0 );
  if ( s.num_operation_positions.at( op_pos ) == 0 )
  {
    Log::get().error( "Error loading operation position counts from stats", true );
  }
  if ( !s.found_programs.at( 4 ) )
  {
    Log::get().error( "Error loading program summary from stats", true );
  }
  if ( s.program_lengths.at( 7 ) != 1 )
  {
    Log::get().error( "Error loading program lengths from stats", true );
  }
  if ( s.cal_graph.count( 40 ) != 1 || s.cal_graph.find( 40 )->second != 10051 )
  {
    Log::get().error( "Unexpected cal in A000040", true );
  }
  if ( s.getTransitiveLength( 40, true ) != 38 )
  {
    Log::get().error( "Unexpected transitive length of A000040", true );
  }

  // save & reload stats
  s.save( "/tmp" );
  t.load( "/tmp" );

  // compare loaded to original
  for ( auto &e : s.num_constants )
  {
    auto m = e.second;
    auto n = t.num_constants.at( e.first );
    if ( m != n )
    {
      Log::get().error( "Unexpected number of constants count: " + std::to_string( m ) + "!=" + std::to_string( n ),
          true );
    }
  }
  for ( size_t i = 0; i < s.num_ops_per_type.size(); i++ )
  {
    auto m = s.num_ops_per_type.at( i );
    auto n = t.num_ops_per_type.at( i );
    if ( m != n )
    {
      Log::get().error(
          "Unexpected number of operation type count: " + std::to_string( m ) + "!=" + std::to_string( n ), true );
    }
  }
  for ( auto it : s.num_operations )
  {
    if ( it.second != t.num_operations.at( it.first ) )
    {
      Log::get().error( "Unexpected number of operations count", true );
    }
  }
  for ( auto it : s.num_operation_positions )
  {
    if ( it.second != t.num_operation_positions.at( it.first ) )
    {
      Log::get().error( "Unexpected number of operation position count", true );
    }
  }
  if ( s.found_programs.size() != t.found_programs.size() )
  {
    Log::get().error(
        "Unexpected number of found programs: " + std::to_string( s.found_programs.size() ) + "!="
            + std::to_string( t.found_programs.size() ), true );
  }
  if ( s.cached_b_files.size() != t.cached_b_files.size() )
  {
    Log::get().error(
        "Unexpected number of cached b-files: " + std::to_string( s.cached_b_files.size() ) + "!="
            + std::to_string( t.cached_b_files.size() ), true );
  }
  for ( size_t i = 0; i < s.found_programs.size(); i++ )
  {
    auto a = s.found_programs.at( i );
    auto b = t.found_programs.at( i );
    if ( a != b )
    {
      Log::get().error( "Unexpected found programs for: " + std::to_string( i ), true );
    }
  }
  for ( size_t i = 0; i < s.cached_b_files.size(); i++ )
  {
    auto a = s.cached_b_files.at( i );
    auto b = t.cached_b_files.at( i );
    if ( a != b )
    {
      Log::get().error( "Unexpected cached b-files for: " + std::to_string( i ), true );
    }
  }
}

void Test::optimizer()
{
  Log::get().info( "Testing optimizer" );
  Settings settings;
  Interpreter interpreter( settings );
  Optimizer optimizer( settings );
  Parser parser;
  size_t i = 1;
  while ( true )
  {
    std::stringstream s;
    s << "tests/optimizer/E" << std::setw( 3 ) << std::setfill( '0' ) << i << ".asm";
    std::ifstream file( s.str() );
    if ( !file.good() )
    {
      break;
    }
    Log::get().info( "Testing " + s.str() );
    auto p = parser.parse( file );
    int in = -1, out = -1;
    for ( size_t i = 0; i < p.ops.size(); i++ )
    {
      if ( p.ops[i].type == Operation::Type::NOP )
      {
        if ( p.ops[i].comment == "in" )
        {
          in = i;
        }
        else if ( p.ops[i].comment == "out" )
        {
          out = i;
        }
      }
    }
    if ( in < 0 || out < 0 || in >= out )
    {
      Log::get().error( "Error parsing test", true );
    }
    Program pin;
    pin.ops.insert( pin.ops.begin(), p.ops.begin() + in + 1, p.ops.begin() + out );
    Program pout;
    pout.ops.insert( pout.ops.begin(), p.ops.begin() + out + 1, p.ops.end() );
    i++;
    Program optimized = pin;
    optimizer.optimize( optimized, 2, 1 );
    if ( optimized != pout )
    {
      ProgramUtil::print( optimized, std::cerr );
      Log::get().error( "Unexpected optimized output", true );
    }
  }
}

void Test::minimizer( size_t tests )
{
  Interpreter interpreter( settings );
  Minimizer minimizer( settings );
  std::random_device rand;
  MultiGenerator multi_generator( settings, manager.getStats(), rand() );
  Sequence s1, s2, s3;
  Program program, minimized;
  for ( size_t i = 0; i < tests; i++ )
  {
    if ( i % (tests / 10) == 0 )
    {
      Log::get().info( "Testing minimizer " + std::to_string( i ) );
    }
    program = multi_generator.getGenerator()->generateProgram();
    multi_generator.next();
    try
    {
      interpreter.eval( program, s1 );
    }
    catch ( const std::exception &e )
    {
      continue;
    }
    minimized = program;
    minimizer.optimizeAndMinimize( minimized, 2, 1, s1.size() );
    interpreter.eval( minimized, s2 );
    if ( s1.size() != s2.size() || (s1 != s2) )
    {
      ProgramUtil::print( program, std::cout );
      Log::get().error( "Program evaluated to different sequence after optimization", true );
    }
  }
}

void Test::linearMatcher()
{
  LinearMatcher matcher( false );
  testMatcherSet( matcher, { 27, 5843, 8585, 16789 } );
  testMatcherSet( matcher, { 290, 1105, 117950 } );
}

void Test::deltaMatcher()
{
  DeltaMatcher matcher( false );
  testMatcherSet( matcher, { 7, 12, 27 } );
  testMatcherSet( matcher, { 108, 14137 } );
  testMatcherSet( matcher, { 4273, 290, 330 } );
  testMatcherSet( matcher, { 168380, 193356 } );
  testMatcherSet( matcher, { 243980, 244050 } );
}

void Test::testBinary( const std::string &func, const std::string &file,
    const std::vector<std::vector<number_t> > &values )
{
  Log::get().info( "Testing " + file );
  Parser parser;
  Settings settings; // need special settings here
// settings needed for ackermann
  settings.max_memory = 100000;
  settings.max_cycles = 10000000;
  Interpreter interpreter( settings );
  auto program = parser.parse( file );
  for ( size_t i = 0; i < values.size(); i++ )
  {
    for ( size_t j = 0; j < values[i].size(); j++ )
    {
      Memory mem;
      mem.set( 0, i );
      mem.set( 1, j );
      interpreter.run( program, mem );
      if ( mem.get( 2 ) != values[i][j] )
      {
        Log::get().error( "unexpected result: " + std::to_string( mem.get( 2 ) ), true );
      }
    }
  }
}

void Test::testSeq( size_t id, const Sequence &expected )
{
  auto file = OeisSequence( id ).getProgramPath();
  Log::get().info( "Testing " + file );
  Parser parser;
  Settings settings; // special settings
  settings.num_terms = expected.size();
  Interpreter interpreter( settings );
  auto p = parser.parse( file );
  Sequence result;
  interpreter.eval( p, result );
  if ( result != expected )
  {
    Log::get().error( "unexpected result: " + result.to_string(), true );
  }
}

void Test::testMatcherSet( Matcher &matcher, const std::vector<size_t> &ids )
{
  for ( auto id1 : ids )
  {
    for ( auto id2 : ids )
    {
      testMatcherPair( matcher, id1, id2 );
    }
  }
}

void eval( const Program& p, Interpreter& interpreter, Sequence& s )
{
  try
  {
    interpreter.eval( p, s );
  }
  catch ( const std::exception& e )
  {
    ProgramUtil::print( p, std::cerr );
    Log::get().error( "Error evaluating program: " + std::string( e.what() ), true );
  }
}

void Test::testMatcherPair( Matcher &matcher, size_t id1, size_t id2 )
{
  Log::get().info(
      "Testing " + matcher.getName() + " matcher for " + OeisSequence( id1 ).id_str() + " -> "
          + OeisSequence( id2 ).id_str() );
  Parser parser;
  Interpreter interpreter( settings );
  auto p1 = parser.parse( OeisSequence( id1 ).getProgramPath() );
  auto p2 = parser.parse( OeisSequence( id2 ).getProgramPath() );
  ProgramUtil::removeOps( p1, Operation::Type::NOP );
  ProgramUtil::removeOps( p2, Operation::Type::NOP );
  Sequence s1, s2, s3;
  eval( p1, interpreter, s1 );
  eval( p2, interpreter, s2 );
  matcher.insert( s2, id2 );
  Matcher::seq_programs_t result;
  matcher.match( p1, s1, result );
  matcher.remove( s2, id2 );
  if ( result.size() != 1 )
  {
    Log::get().error( matcher.getName() + " matcher unable to match sequence", true );
  }
  if ( result[0].first != id2 )
  {
    Log::get().error( matcher.getName() + " matcher returned unexpected sequence ID", true );
  }
  eval( result[0].second, interpreter, s3 );
  if ( s2.size() != s3.size() || s2 != s3 )
  {
    ProgramUtil::print( result[0].second, std::cout );
    Log::get().error( matcher.getName() + " matcher generated wrong program for " + OeisSequence( id2 ).id_str(),
        true );
  }
}
