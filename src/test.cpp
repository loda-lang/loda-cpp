#include "test.hpp"

#include "generator.hpp"
#include "interpreter.hpp"
#include "iterator.hpp"
#include "number.hpp"
#include "oeis.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "synthesizer.hpp"

#include <deque>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <stdexcept>

void Test::all()
{
  exponentiation();
  ackermann();
  iterator();
  polynomial_synthesizer( 10000, 0 );
  polynomial_synthesizer( 1000, 1 );
  polynomial_matcher( 10000, 0 );
  polynomial_matcher( 10000, 1 );
  polynomial_matcher( 10000, 2 );
  optimizer( 1000 );
}

void Test::exponentiation()
{
  std::vector<std::vector<number_t> > values = { { 1, 0, 0, 0 }, { 1, 1, 1, 1 }, { 1, 2, 4, 8 }, { 1, 3, 9, 27 }, { 1,
      4, 16, 64 } };
  testBinary( "exp", "programs/exponentiation.asm", values );
}

void Test::ackermann()
{
  std::vector<std::vector<number_t> > values = { { 1, 2, 3, 4, 5 }, { 2, 3, 4, 5, 6 }, { 3, 5, 7, 9, 11 }, { 5, 13, 29,
      61, 125 }, { 13, 65533 } };
  testBinary( "ack", "programs/ackermann.asm", values );
}

void Test::iterator()
{
  Log::get().info( "Testing iterator" );
  Iterator it;
//  Printer printer;
  for ( int i = 0; i < 100000; i++ )
  {
    if ( exit_flag_ ) break;
    it.next();
//    std::cout << "\x1B[2J\x1B[H";
//    printer.print(next, std::cout );
//    std::cin.ignore();
  }
}

void Test::optimizer( size_t tests )
{
  Settings settings;
  Interpreter interpreter( settings );
  Optimizer optimizer( settings );
  Generator generator( settings, 5, std::random_device()() );
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
      Log::get().error( "Program evaluated to different sequence after optimization", true );
    }
  }
}

void Test::polynomial_matcher( size_t tests, size_t degree )
{
  Settings settings;
  Parser parser;
  Interpreter interpreter( settings );
  Optimizer optimizer( settings );
  std::random_device rand_dev;
  PolynomialMatcher matcher;
  Log::get().info( "Testing polynomial matcher for degree " + std::to_string( degree ) );

  // load test program
  std::vector<number_t> program_ids = { 4, 35, 2262 };
  std::vector<Program> programs;
  for ( number_t id : program_ids )
  {
    auto program = parser.parse( "programs/oeis/" + OeisSequence( id ).id_str() + ".asm" );
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
        target_seq[n] += pol[d] * pow( n, d );
      }
    }
    Matcher::seq_programs_t results;
    matcher.insert( target_seq, i );
    matcher.match( program, norm_seq, results );
    if ( results.size() != 1 )
    {
      Printer r;
      r.print( program, std::cout );
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
      Printer r;
      std::cout << "# Input program: " << std::endl;
      r.print( program, std::cout );
      std::cout << std::endl << "# Output program: " << std::endl;
      r.print( results[0].second, std::cout );
      std::cout << "# Target sequence: " + target_seq.to_string() << std::endl;
      std::cout << "# Output sequence: " + result_seq.to_string() << std::endl;
      Log::get().error( "Error: matched program yields wrong unexpected result", true );
    }
    matcher.remove( target_seq, i );
  }
}

void Test::polynomial_synthesizer( size_t tests, size_t degree )
{
  Log::get().info( "Testing polynomial synthesizer for degree " + std::to_string( degree ) );
  std::random_device rand_dev;
  Settings settings;
  LinearSynthesizer synth;
  Interpreter interpreter( settings );
  Printer printer;
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
      printer.print( prog, std::cout );
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
    Log::get().error( "unexpected result", true );
  }
}
