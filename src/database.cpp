#include "database.hpp"

#include "interpreter.hpp"
#include "optimizer.hpp"
#include "printer.hpp"
#include "serializer.hpp"

#include <fstream>

class Reader
{
public:

  Reader()
  {
    db_.open( "loda.db", std::ios::in | std::ios::binary );
  }

  bool next( Program& p )
  {
    if ( !db_.good() || db_.eof() )
    {
      return false;
    }
    s_.readProgram( p, db_ );
    return true;
  }

private:
  std::ifstream db_;
  Serializer s_;

};

bool Database::insert( Program&& p )
{
  Optimizer o;
  o.optimize( p );

  Interpreter i;
  Sequence s = i.eval( p, 32 );
  if ( s.linear() )
  {
    return false;
  }
  programs_.push_back( std::make_pair<Program, Sequence>( Program( p ), std::move( s ) ) );
  return true;
}

void Database::save()
{
  std::sort( programs_.begin(), programs_.end(),
      [](const std::pair<Program,Sequence>& a, const std::pair<Program,Sequence>& b) -> bool
      { return a.second < b.second;} );

  std::ofstream new_db;
  new_db.open( "loda_new.db", std::ios::out | std::ios::binary );
  if ( !new_db.good() )
  {
    throw std::runtime_error( "write error" );
  }

  Optimizer o;
  Interpreter i;
  Reader r;
  Serializer s;
  auto p = programs_.begin();
  Program q;
  bool db_has_next = r.next( q );
  Program last;
  while ( db_has_next || p != programs_.end() )
  {
    Program next;
    Sequence seq;
    if ( db_has_next && p != programs_.end() )
    {
      auto e = i.eval( q, 32 );
      if ( e < p->second )
      {
        seq = e;
        next = q;
        db_has_next = r.next( q );
      }
      else
      {
        seq = p->second;
        next = p->first;
        ++p;
      }
    }
    else if ( db_has_next )
    {
      auto e = i.eval( q, 32 );
      seq = e;
      next = q;
      db_has_next = r.next( q );
    }
    else
    {
      seq = p->second;
      next = p->first;
      ++p;
    }

    o.optimize( next );

    if ( seq.subsequence( 8 ).linear() )
    {
      std::cout << "removing linear program" << std::endl;
    }
    else if ( next == last )
    {
      std::cout << "removing duplicate program" << std::endl;
    }
    else
    {
      s.writeProgram( next, new_db );
      last = next;
    }
  }
  new_db.flush();
  new_db.close();
  rename( "loda_new.db", "loda.db" );
}

void Database::printPrograms()
{
  Reader r;
  Program p;
  Printer q;
  number_t n = 1;
  while ( r.next( p ) )
  {
    if ( n > 1 )
    {
      std::cout << std::endl;
    }
    std::cout << "# program " << n++ << std::endl;
    q.print( p, std::cout );
  }
}

void Database::printSequences()
{
  Reader r;
  Program p;
  Interpreter i;
  while ( r.next( p ) )
  {
    std::cout << i.eval( p, 32 ) << std::endl;
  }
}
