#include "test.hpp"

#include "generator.hpp"
#include "interpreter.hpp"
#include "matcher.hpp"
#include "miner.hpp"
#include "number.hpp"
#include "oeis.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "semantics.hpp"
#include "synthesizer.hpp"

#include <deque>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <stdexcept>

void Test::all()
{
  fibonacci();
  ackermann();
  collatz();
  deltaMatcher();
  polynomialSynthesizer( 10000, 0 );
  polynomialSynthesizer( 1000, 1 );
  for ( int d = 0; d <= PolynomialMatcher::DEGREE; d++ )
  {
    polynomialMatcher( 10000, d );
  }
  optimizer( 10000 );
}

std::string getOeisFile( number_t id )
{
  return "programs/oeis/" + OeisSequence( id ).id_str() + ".asm";
}

void Test::fibonacci()
{
  Sequence values( { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233 } );
  testSeq( "fib", getOeisFile( 45 ), values );
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

void Test::optimizer( size_t tests )
{
  Settings settings;
  Interpreter interpreter( settings );
  Optimizer optimizer( settings );
  Generator generator( settings, std::random_device()() );
  Sequence s1, s2, s3;
  Program program, optimized, minimized;
  Log::get().info( "Testing optimizer and minimizer" );
  for ( size_t i = 0; i < tests; i++ )
  {
    if ( exit_flag_ ) break;
    program = generator.generateProgram();
    try
    {
      s1 = interpreter.eval( program );
    }
    catch ( const std::exception &e )
    {
      continue;
    }
    optimized = program;
    optimizer.optimize( optimized, 2, 1 );
    s2 = interpreter.eval( optimized );
    if ( s1.size() != s2.size() || (s1 != s2) )
    {
      ProgramUtil::print( program, std::cout );
      Log::get().error( "Program evaluated to different sequence after optimization", true );
    }
  }
}

void Test::deltaMatcher()
{
  DeltaMatcher matcher;
  matcher.setBackOff( false );
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
  PolynomialMatcher matcher;
  matcher.setBackOff( false );
  Log::get().info( "Testing polynomial matcher for degree " + std::to_string( degree ) );

  // load test program
  std::vector<number_t> program_ids = { 4, 35, 2262 };
  std::vector<Program> programs;
  for ( number_t id : program_ids )
  {
    auto program = parser.parse( getOeisFile( id ) );
    optimizer.removeNops( program );
    programs.push_back( program );
  }

  // run matcher tests
  for ( number_t i = 0; i < tests; i++ )
  {
    if ( exit_flag_ ) break;

    // evaluate test program
    auto program = programs[i % program_ids.size()];
    auto norm_seq = interpreter.eval( program );

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
      result_seq = interpreter.eval( results[0].second );
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

void Test::polynomialSynthesizer( size_t tests, size_t degree )
{
  Log::get().info( "Testing polynomial synthesizer for degree " + std::to_string( degree ) );
  std::random_device rand_dev;
  Settings settings;
  LinearSynthesizer synth;
  Interpreter interpreter( settings );
  Program prog;
  for ( size_t i = 0; i < tests; i++ )
  {
    if ( exit_flag_ ) break;
    Polynomial pol( degree );
    for ( size_t d = 0; d < pol.size(); d++ )
    {
      pol[d] = rand_dev() % 1000;
      pol[d] = pol[d] > 0 ? pol[d] : -pol[d];
    }
    Log::get().debug( "Checking polynomial " + pol.to_string() );
    auto seq1 = pol.eval( settings.num_terms );
    if ( !synth.synthesize( seq1, prog ) )
    {
      Log::get().error(
          "Error synthesizing program for polynomial " + pol.to_string() + ", target sequence: " + seq1.to_string(),
          true );
    }
    auto seq2 = interpreter.eval( prog );
    if ( seq1 != seq2 )
    {
      ProgramUtil::print( prog, std::cout );
      Log::get().error( "Synthesized program for polynomial " + pol.to_string() + " yields incorrect result", true );
    }
  }
}

void Test::testBinary( const std::string &func, const std::string &file,
    const std::vector<std::vector<number_t> > &values )
{
  Log::get().info( "Testing " + file );
  Parser parser;
  Settings settings;
  Interpreter interpreter( settings );
  auto program = parser.parse( file );
  for ( size_t i = 0; i < values.size(); i++ )
  {
    for ( size_t j = 0; j < values[i].size(); j++ )
    {
      if ( exit_flag_ ) break;
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

void Test::testSeq( const std::string &func, const std::string &file, const Sequence &expected )
{
  Log::get().info( "Testing " + file );
  Parser parser;
  Settings settings;
  settings.num_terms = expected.size();
  Interpreter interpreter( settings );
  auto p = parser.parse( file );
  auto result = interpreter.eval( p );
  if ( result != expected )
  {
    Log::get().error( "unexpected result: " + result.to_string(), true );
  }
}

void Test::testMatcherSet( Matcher &matcher, const std::vector<number_t> &ids )
{
  for ( auto id1 : ids )
  {
    for ( auto id2 : ids )
    {
      testMatcherPair( matcher, id1, id2 );
    }
  }
}

void Test::testMatcherPair( Matcher &matcher, number_t id1, number_t id2 )
{
  Log::get().info(
      "Testing " + matcher.getName() + " matcher for " + OeisSequence( id1 ).id_str() + " -> "
          + OeisSequence( id2 ).id_str() );
  Parser parser;
  Settings settings;
  Interpreter interpreter( settings );
  auto p1 = parser.parse( getOeisFile( id1 ) );
  auto p2 = parser.parse( getOeisFile( id2 ) );
  ProgramUtil::removeOps( p1, Operation::Type::NOP );
  ProgramUtil::removeOps( p2, Operation::Type::NOP );
  auto s1 = interpreter.eval( p1 );
  auto s2 = interpreter.eval( p2 );
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
  auto s3 = interpreter.eval( result[0].second );
  if ( s2.size() != s3.size() || s2 != s3 )
  {
    ProgramUtil::print( result[0].second, std::cout );
    Log::get().error( matcher.getName() + " matcher generated wrong program for " + OeisSequence( id2 ).id_str(),
        false );
  }
}
