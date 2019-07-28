#include "test.hpp"

#include "generator.hpp"
#include "interpreter.hpp"
#include "iterator.hpp"
#include "number.hpp"
#include "oeis.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "printer.hpp"

#include <deque>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <stdexcept>

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

void Test::iterate()
{
  Iterator it;
  Printer printer;
  while ( true )
  {
    std::cout << "\x1B[2J\x1B[H";
    printer.print( it.next(), std::cout );
//    std::cin.ignore();
  }
}

void Test::optimize()
{
  Settings settings;
  Interpreter interpreter( settings );
  Optimizer optimizer( settings );
  Generator generator( settings, 5, std::random_device()() );
  Sequence s1, s2, s3;
  Program program, optimized, minimized;

  Log::get().info( "Testing optimization and minimization" );
  for ( size_t i = 0; i < 10000; i++ )
  {
    program = generator.generateProgram();
    try
    {
      s1 = interpreter.eval( program );
    }
    catch ( const std::exception& e )
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

void Test::oeis()
{
  Settings settings;
  Oeis o( settings );
  o.load();
  std::ifstream readme_in( "README.md" );
  std::stringstream buffer;
  std::string str;
  while ( std::getline( readme_in, str ) )
  {
    buffer << str << std::endl;
    if ( str == "## Available Programs" )
    {
      break;
    }
  }
  readme_in.close();
  std::ofstream readme_out( "README.md" );
  readme_out << buffer.str() << std::endl;
  std::ofstream list_file;
  int list_index = -1;
  size_t num_programs = 0;
  for ( auto& s : o.getSequences() )
  {
    std::string file_name = "programs/oeis/" + s.id_str() + ".asm";
    std::ifstream file( file_name );
    if ( file.good() )
    {
      num_programs++;
      Log::get().info( "Checking first " + std::to_string( s.full.size() ) + " terms of " + s.to_string() );
      Parser parser;
      Program program = parser.parse( file );
      Settings settings2 = settings;
      settings2.num_terms = s.full.size();
      Interpreter interpreter( settings2 );
      bool okay;
      try
      {
        Sequence result = interpreter.eval( program );
        if ( result.size() != s.full.size() || result != s.full )
        {
          Log::get().error( "Program did not evaluate to expected sequence" );
          okay = false;
        }
        else
        {
          okay = true;
        }
      }
      catch ( const std::exception& exc )
      {
        okay = false;
      }
      if ( !okay )
      {
        Log::get().warn( "Deleting " + file_name );
        file.close();
        remove( file_name.c_str() );
      }
      else
      {
        Log::get().info( "Optimizing and minimizing " + file_name );
        program.removeOps( Operation::Type::NOP );
        Program optimized = program;
        Optimizer optimizer( settings2 );
        optimizer.minimize( optimized, s.full.size() );
        optimizer.optimize( optimized, 2, 1 );
        if ( !(program == optimized) )
        {
          Log::get().warn( "Program not optimal! Updating ..." );
        }
        o.dumpProgram( s.id, optimized, file_name );
        if ( list_index < 0 || (int) s.id / 100000 != list_index )
        {
          list_index++;
          std::string list_path = "programs/oeis/list" + std::to_string( list_index ) + ".md";
          OeisSequence start( (list_index * 100000) + 1 );
          OeisSequence end( (list_index + 1) * 100000 );
          readme_out << "* [" << start.id_str() << "-" << end.id_str() << "](" << list_path << ")\n";
          list_file.close();
          list_file.open( list_path );
          list_file << "# Programs for " << start.id_str() << "-" << end.id_str() << "\n\n";
          list_file
              << "List of integer sequences with links to LODA programs. An _Ln_ program is a LODA program of length _n_."
              << "\n\n";
        }
        list_file << "* [" << s.id_str() << "](http://oeis.org/" << s.id_str() << ") ([L" << std::setw( 2 )
            << std::setfill( '0' ) << optimized.num_ops( false ) << " program](" << s.id_str() << ".asm)): " << s.name
            << "\n";
      }
    }
  }
  list_file.close();
  readme_out << "\n" << "Total number of programs: ";
  readme_out << num_programs << "/" << o.getTotalCount() << " (" << (int) (100 * num_programs / o.getTotalCount())
      << "%)\n\n";
  readme_out
      << "![LODA Program Length Distribution](https://raw.githubusercontent.com/ckrause/loda/master/lengths.png)\n";
  readme_out << "![LODA Program Counts](https://raw.githubusercontent.com/ckrause/loda/master/counts.png)\n";
  readme_out.close();
  std::cout << std::endl;
}

void Test::primes()
{
  int n = 1000, i = 3, count, c;
  std::vector<size_t> primes;
  primes.push_back( 2 );
  for ( count = 2; count <= n; )
  {
    for ( c = 2; c <= i - 1; c++ )
    {
      if ( i % c == 0 ) break;
    }
    if ( c == i )
    {
      primes.push_back( i );
      count++;
    }
    i++;
  }

  size_t index = 0;
  for ( auto p : primes )
  {
    /*    std::cout << "P= " << p << std::endl;
     std::cout << "R= ";
     for ( auto r : primes )
     {
     std::cout << (p % r) << " ";
     if ( r == p ) break;
     }
     std::cout << std::endl;
     */if ( p == primes.back() ) continue;
    size_t diff = primes.at( index + 1 ) - p;
    std::cout << "D(" << p << ")= ";
    for ( auto d : primes )
    {
      if ( (diff / d) == 0 ) break;
      std::cout << (diff / d) << " ";
      if ( d == p ) break;
    }
//    std::cout << std::endl;
    std::cout << std::endl;
    index++;
  }
}

void Test::primes2()
{
  int prime = 1; // will store current prime number
  int gap = 1; // stores the gap to the next prime number
  std::deque<int> next_gaps = { 1 }; // list of next gap

  for ( int i = 1; i <= 100; i++ )
  {
    prime += gap; // next prime is current prime plus gap
    gap = next_gaps.front(); // use next gap from list

    next_gaps.pop_front(); // move next gap from front...
    next_gaps.push_back( gap ); // ...to end

    std::deque<int> updated_gaps;

    // make prime number copies of the list
    if ( next_gaps.size() < 1000 )
    {
      for ( int j = 0; j < prime; j++ )
      {
        std::copy( next_gaps.begin(), next_gaps.end(), std::back_inserter( updated_gaps ) );
      }
    }

    // remove illegal gaps from the list
    int sum = prime + gap;
    for ( int j = 0; j < (int) updated_gaps.size(); j++ )
    {
      sum = (sum + updated_gaps[j]) % prime;
      if ( sum == 0 )
      {
        sum = (sum + updated_gaps[j + 1]) % prime;
        updated_gaps[j] += updated_gaps[j + 1];
        updated_gaps.erase( updated_gaps.begin() + j + 1, updated_gaps.begin() + j + 2 );
      }
    }
    next_gaps = updated_gaps;

    std::cout << "Step " << i << ": p=" << prime << "; g=" << gap << std::endl;

  }
}

void Test::matcher()
{
  Settings settings;
  settings.operand_types = "cd";
  settings.num_operations = 10;
  Interpreter interpreter( settings );
  Optimizer optimizer( settings );
  std::random_device rand_dev;
  Generator generator( settings, 5, rand_dev() );
  LinearMatcher matcher;

  Log::get().info( "Testing matcher" );
  for ( number_t i=0; i < 10000; i++ )
  {
    // generate a program
    auto program = generator.generateProgram();
    std::unordered_set<number_t> used_cells;
    number_t largest_used = 0;
    if ( !optimizer.getUsedMemoryCells( program, used_cells, largest_used ) )
    {
      continue;
    }

    // evaluate program
    Sequence norm_seq;
    try
    {
      norm_seq = interpreter.eval( program );
    }
    catch ( const std::exception& )
    {
      continue;
    }
    if ( norm_seq.is_linear() || norm_seq.min() != norm_seq[0] )
    {
      continue;
    }

    // test matcher
    int64_t slope = rand_dev() % 1000;
    slope = slope > 0 ? slope : -slope;
    int64_t offset = rand_dev() % 1000;
    offset = offset > 0 ? offset : -offset;

    // std::cout << std::endl;
    Log::get().info( "Testing sequence (" + norm_seq.to_string() + ") + " + std::to_string(slope) + "n + " + std::to_string(offset) );
    matcher.insert( norm_seq, i );
    auto target_seq = norm_seq;
    for (size_t n=0; n < target_seq.size(); n++)
    {
      target_seq[n] += slope * n + offset;
    }
    Matcher::seq_programs_t results;
    matcher.match( program, target_seq, results );
    if ( results.size() != 1 )
    {
      Printer r;
      r.print( program , std::cout );
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
      std::cout << "Input program: " << std::endl;
      r.print( program , std::cout );

      std::cout << std::endl << "Output program: " << std::endl;
      r.print( program , std::cout );
      std::cout << "Target sequence: " + target_seq.to_string() << std::endl;
      std::cout << "Output sequence: " + result_seq.to_string() << std::endl;
      Log::get().error( "Error: matched program yields wrong unexpected result", true );
    }
    matcher.remove( norm_seq, i );
  }
}

void Test::all()
{
  matcher();
//  primes2();
//  iterate();
//  primes();
  exponentiation();
  ackermann();
  optimize();
  oeis();
}

void Test::testBinary( const std::string& func, const std::string& file,
    const std::vector<std::vector<number_t> >& values )
{
  std::cout << "Running tests for " << file << "..." << std::endl;
  Parser parser;
  Settings settings;
  Interpreter interpreter( settings );
  auto program = parser.parse( file );
  for ( size_t i = 0; i < values.size(); i++ )
  {
    for ( size_t j = 0; j < values[i].size(); j++ )
    {
      std::cout << func << "(" << i << "," << j << ")=";
      Memory mem;
      mem.set( 0, i );
      mem.set( 1, j );
      interpreter.run( program, mem );
      std::cout << mem.get( 2 ) << std::endl;
      if ( mem.get( 2 ) != values[i][j] )
      {
        throw std::runtime_error( "unexpected result: " + std::to_string( mem.get( 2 ) ) );
      }
    }
  }
  std::cout << std::endl;
}

void Test::testSeq( const std::string& func, const std::string& file, const Sequence& expected )
{
  std::cout << "Running tests for " + file + "..." << std::endl;

  Parser parser;
  Settings settings;
  settings.num_terms = expected.size();
  Interpreter interpreter( settings );

  auto fib = parser.parse( file );
  auto result = interpreter.eval( fib );
  std::cout << func << "=" << result << "..." << std::endl;
  if ( result != expected )
  {
    throw std::runtime_error( "unexpected result" );
  }
  std::cout << std::endl;

}
