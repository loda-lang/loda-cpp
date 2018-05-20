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

  while ( true )
  {
    p = m.generateProgram( 0 );
    auto result = evaluator.Eval( *p, target.Length() );
    auto score = scorer.Score( result );
    if ( score == 0 )
    {
      std::cout << "Found!" << std::endl;
      printer.Print( *p, std::cout );
      return p;
    }
    std::cout << score << ":: " << result << std::endl;
    m.mutate( 0.5 );
  }

}
