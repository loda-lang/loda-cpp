#include "test.hpp"

#include "interpreter.hpp"
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

void Test::all()
{
  semantics();
  memory();
  config();
  stats();
  knownPrograms();
  ackermann();
  collatz();
  linearMatcher();
  deltaMatcher();
  size_t tests = 100;
  for ( size_t degree = 0; degree <= (size_t) PolynomialMatcher::DEGREE; degree++ )
  {
    polynomialMatcher( tests, degree );
  }
  optimizer();
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

void Test::knownPrograms()
{
  testSeq( 5, Sequence( { 1, 2, 2, 3, 2, 4, 2, 4, 3, 4, 2, 6, 2, 4, 4, 5, 2, 6, 2, 6 } ) );
  testSeq( 30, Sequence( { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 } ) );
  testSeq( 45, Sequence( { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181 } ) );
  testSeq( 79, Sequence( { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536 } ) );
  testSeq( 1489, Sequence( { 0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16, -17 } ) );
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

void check_int( const std::string &s, int64_t min, int64_t max, int64_t value )
{
  if ( value < min || value > max )
  {
    Log::get().error(
        "Expected '" + s + "' min: " + std::to_string( min ) + ", max: " + std::to_string( max ) + ", got: "
            + std::to_string( value ), true );
  }
}

void Test::config()
{
  Log::get().info( "Testing config" );
  std::ifstream in( "loda.json" );
  auto configs = Generator::Config::load( in, false );
  check_int( "numGenerators", 2, 20, configs.size() );
  for ( auto &c : configs )
  {
    check_int( "version", 1, 3, c.version );
    check_int( "replicas", 1, 3, c.replicas );
    if ( c.version == 1 )
    {
      check_int( "length", 20, 120, c.length );
      check_int( "max_constant", 4, 10, c.max_constant );
      check_int( "max_index", 4, 10, c.max_index );
      check_int( "loops", 0, 1, c.loops );
      check_int( "indirectAccess", 0, 1, c.indirect_access );
    }
  }
}

void Test::stats()
{
  Log::get().info( "Testing stats loading and saving" );

  // load stats
  Stats s, t;
  s.load( "stats" );

  // sanity check for loaded stats
  if ( s.num_constants.at( 1 ) == 0 )
  {
    Log::get().error( "Error loading constants counts from stats" );
  }
  if ( s.num_ops_per_type.at( static_cast<size_t>( Operation::Type::MOV ) ) == 0 )
  {
    Log::get().error( "Error loading operation type counts from stats" );
  }
  Operation op( Operation::Type::ADD, Operand( Operand::Type::DIRECT, 0 ), Operand( Operand::Type::CONSTANT, 1 ) );
  if ( s.num_operations.at( op ) == 0 )
  {
    Log::get().error( "Error loading operation counts from stats" );
  }
  if ( s.num_operation_positions.size() < 100000 )
  {
    Log::get().error( "Unexpected number of operation position counts in stats" );
  }
  OpPos op_pos;
  op_pos.pos = 0;
  op_pos.len = 2;
  op_pos.op.type = Operation::Type::MOV;
  op_pos.op.target = Operand( Operand::Type::DIRECT, 1 );
  op_pos.op.source = Operand( Operand::Type::DIRECT, 0 );
  if ( s.num_operation_positions.at( op_pos ) == 0 )
  {
    Log::get().error( "Error loading operation position counts from stats" );
  }
  if ( !s.found_programs.at( 4 ) || !s.cached_b_files.at( 4 ) )
  {
    Log::get().error( "Error loading program summary from stats" );
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
      Log::get().error( "Unexpected optimized output" );
    }
  }
}

void Test::minimizer( size_t tests )
{
  Settings settings;
  Interpreter interpreter( settings );
  Minimizer minimizer( settings );
  std::random_device rand;
  MultiGenerator multi_generator( settings, rand() );
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
  testMatcherSet( matcher, { 4273, 290, 330 } );
  testMatcherSet( matcher, { 168380, 193356 } );
}

void Test::polynomialMatcher( size_t tests, size_t degree )
{
  Settings settings;
  Parser parser;
  Interpreter interpreter( settings );
  Optimizer optimizer( settings );
  std::random_device rand_dev;
  PolynomialMatcher matcher( false );
  Log::get().info( "Testing polynomial matcher for degree " + std::to_string( degree ) );

// load test program
  std::vector<size_t> program_ids = { 4, 35, 2262 };
  std::vector<Program> programs;
  for ( auto id : program_ids )
  {
    auto program = parser.parse( OeisSequence( id ).getProgramPath() );
    optimizer.removeNops( program );
    programs.push_back( program );
  }

// run matcher tests
  for ( size_t i = 0; i < tests; i++ )
  {
    // evaluate test program
    auto program = programs[i % program_ids.size()];
    Sequence norm_seq;
    interpreter.eval( program, norm_seq );

    // test polynomial matcher
    Polynomial pol( degree );
    for ( size_t d = 0; d < pol.size(); d++ )
    {
      pol[d] = rand_dev() % 100;
    }

    Log::get().debug( "Checking (" + norm_seq.to_string() + ") + " + pol.to_string() );
    auto target_seq = norm_seq;
    for ( size_t n = 0; n < target_seq.size(); n++ )
    {
      for ( size_t d = 0; d < pol.size(); d++ )
      {
        target_seq[n] += pol[d] * Semantics::pow( n, d );
      }
    }
    Matcher::seq_programs_t results;
    matcher.insert( target_seq, i );
    matcher.match( program, norm_seq, results );
    if ( results.size() != 1 )
    {
      ProgramUtil::print( program, std::cout );
      Log::get().error( "Error: no program found", true );
    }
    Sequence result_seq;
    try
    {
      interpreter.eval( results[0].second, result_seq );
    }
    catch ( const std::exception& )
    {
      Log::get().error( "Error evaluating generated program", true );
    }
    if ( result_seq != target_seq )
    {
      std::cout << "# Input program: " << std::endl;
      ProgramUtil::print( program, std::cout );
      std::cout << std::endl << "# Output program: " << std::endl;
      ProgramUtil::print( results[0].second, std::cout );
      std::cout << "# Target sequence: " + target_seq.to_string() << std::endl;
      std::cout << "# Output sequence: " + result_seq.to_string() << std::endl;
      Log::get().error( "Error: matched program yields wrong unexpected result", true );
    }
    matcher.remove( target_seq, i );
  }
}

void Test::testBinary( const std::string &func, const std::string &file,
    const std::vector<std::vector<number_t> > &values )
{
  Log::get().info( "Testing " + file );
  Parser parser;
  Settings settings;
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
  Settings settings;
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

void Test::testMatcherPair( Matcher &matcher, size_t id1, size_t id2 )
{
  Log::get().info(
      "Testing " + matcher.getName() + " matcher for " + OeisSequence( id1 ).id_str() + " -> "
          + OeisSequence( id2 ).id_str() );
  Parser parser;
  Settings settings;
  Interpreter interpreter( settings );
  auto p1 = parser.parse( OeisSequence( id1 ).getProgramPath() );
  auto p2 = parser.parse( OeisSequence( id2 ).getProgramPath() );
  ProgramUtil::removeOps( p1, Operation::Type::NOP );
  ProgramUtil::removeOps( p2, Operation::Type::NOP );
  Sequence s1, s2, s3;
  interpreter.eval( p1, s1 );
  interpreter.eval( p2, s2 );
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
  interpreter.eval( result[0].second, s3 );
  if ( s2.size() != s3.size() || s2 != s3 )
  {
    ProgramUtil::print( result[0].second, std::cout );
    Log::get().error( matcher.getName() + " matcher generated wrong program for " + OeisSequence( id2 ).id_str(),
        false );
  }
}
