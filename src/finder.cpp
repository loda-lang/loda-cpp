#include "finder.hpp"

#include "generator.hpp"
#include "evaluator.hpp"
#include "scorer.hpp"
#include "printer.hpp"

Program::UPtr Finder::Find( const Sequence& target )
{
  Program::UPtr p;

  Generator m( 10, 951 );
  Evaluator evaluator;
  FixedSequenceScorer scorer( target );
  Printer printer;

  Sequence s;
  while ( true )
  {
    p = m.generateProgram( 0 );
    try
    {
      s = evaluator.Eval( *p, target.Length() );
    }
    catch ( const std::exception& e )
    {
      std::cerr << std::string( e.what() ) << std::endl;
      continue;
    }
    auto score = scorer.Score( s );
    if ( score == 0 )
    {
      std::cout << "Found!" << std::endl;
      printer.Print( *p, std::cout );
      return p;
    }
    std::cout << score << ":: " << s << std::endl;
    m.mutate( 0.5 );
  }

}
