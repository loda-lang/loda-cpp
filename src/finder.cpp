#include "finder.hpp"

#include "generator.hpp"
#include "interpreter.hpp"
#include "scorer.hpp"
#include "printer.hpp"

Program Finder::find( const Sequence& target )
{
  Program p;

  number_t seed = 23;
  size_t states = 10;
  size_t gen_count = 100;
  size_t tries = 100;

  number_t max_value = 0;
  for ( auto& v : target.data )
  {
    if ( v > max_value ) max_value = v;
  }

  std::vector<Generator::UPtr> generators;
  for ( size_t i = 0; i < gen_count; i++ )
  {
    generators.emplace_back( Generator::UPtr( new Generator( states, seed + i ) ) );
  }

  Interpreter interpreter;
  FixedSequenceScorer scorer( target );
  Printer printer;

  Sequence s;
  while ( true )
  {
    for ( size_t i = 0; i < gen_count; i++ )
    {
      generators[i]->setSeed( seed + i );
    }

    // evaluate generator and score them
    double avg_score = 0.0;
    for ( auto& gen : generators )
    {
      gen->score = 0;
      for ( size_t i = 0; i < tries; i++ )
      {
        p = gen->generateProgram( 0 );
        try
        {
          s = interpreter.eval( p, target.size() );
        }
        catch ( const std::exception& e )
        {
          std::cerr << std::string( e.what() ) << std::endl;
//          gen->score += 2 * max_value;
          continue;
        }
        auto score = scorer.score( s );
        if ( score == 0 )
        {
          std::cout << "Found!" << std::endl;
          printer.print( p, std::cout );
          return p;
        }
        if ( gen->score == 0 || score << gen->score )
        {
          gen->score = score;
        }
      }
//      std::cout << "Score: " << gen->score << std::endl;
      avg_score += gen->score;
    }
    avg_score = (avg_score / generators.size()) / tries;
//    std::cout << "Average: " << avg_score << "\n" << std::endl;

// sort generators by their scores
    std::stable_sort( generators.begin(), generators.end(), less_than_score );

    // print top ten
    for ( size_t i = 0; i < (gen_count / 10); i++ )
    {
      generators[i]->print();
      std::cout << std::endl;
    }
    std::cout << std::endl;

    // create new generators
    std::vector<Generator::UPtr> new_generators;
    for ( size_t i = 0; i < (gen_count / 10); i++ )
    {
      for ( size_t j = 0; j < (gen_count / 10); j++ )
      {
        Generator::UPtr g( new Generator( *generators[i] + *generators[j] ) );
        g->mutate( 0.5 );
        new_generators.emplace_back( std::move( g ) );
      }
    }
    generators = std::move( new_generators );
  }

}
