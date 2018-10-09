#include "database.hpp"

#include "interpreter.hpp"
#include "optimizer.hpp"
#include "printer.hpp"
#include "serializer.hpp"
#include "util.hpp"

#include <fstream>
#include <sstream>

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

Database::Database( const Settings& settings )
    : settings( settings ),
      dirty( true )
{
}

bool Database::insert( Program&& p )
{
  Optimizer o;
  o.optimize( p, 1 );

  Interpreter i( settings );
  Sequence s = i.eval( p );
  if ( !programs.empty() && programs.back().second == s )
  {
    return false;
  }

  //std::stringstream buf;
  //buf << "Inserting sequence " << s;
  //Log::get().info( buf.str() );
  programs.push_back( std::make_pair<Program, Sequence>( Program( p ), std::move( s ) ) );
  dirty = true;
  if ( programs.size() >= 1 )
  {
    save();
  }
  return true;
}

void Database::save()
{
  // nothing to do ?
  if ( !dirty && programs.empty() )
  {
    return;
  }

  // sort programs by lexigographical order of sequences
  std::sort( programs.begin(), programs.end(),
      [](const std::pair<Program,Sequence>& a, const std::pair<Program,Sequence>& b) -> bool
      { return a.second < b.second;} );

  // open temporary file
  std::ofstream new_db;
  new_db.open( "loda_new.db", std::ios::out | std::ios::binary );
  if ( !new_db.good() )
  {
    Log::get().error( "Error write to file: loda_new.db", true );
  }

  Optimizer optimizer;
  Interpreter interpreter( settings );
  Reader reader;
  Serializer serializer;
  auto program1 = programs.begin();
  Program program2;
  bool db_has_next = reader.next( program2 );
  Program last_program;
  Sequence last_sequence;
  Sequence tmp_sequence;
  size_t program_count = 0;
  while ( db_has_next || program1 != programs.end() )
  {
    Program next_program;
    Sequence next_sequence;
    if ( db_has_next && program1 != programs.end() )
    {
      tmp_sequence = interpreter.eval( program2 );
      if ( tmp_sequence < program1->second )
      {
        next_sequence = tmp_sequence;
        next_program = program2;
        db_has_next = reader.next( program2 );
      }
      else
      {
        next_sequence = program1->second;
        next_program = program1->first;
        ++program1;
      }
    }
    else if ( db_has_next )
    {
      next_sequence = interpreter.eval( program2 );
      next_program = program2;
      db_has_next = reader.next( program2 );
    }
    else
    {
      next_sequence = program1->second;
      next_program = program1->first;
      ++program1;
    }

    optimizer.optimize( next_program, 1 );

    if ( next_sequence == last_sequence )
    {
      Log::get().warn( "Removing program for duplicate sequence" );
    }
    else if ( next_program == last_program )
    {
      Log::get().warn( "Removing duplicate program" );
    }
    else
    {
      serializer.writeProgram( next_program, new_db );
      last_program = next_program;
      ++program_count;
    }
  }
  new_db.flush();
  new_db.close();
  rename( "loda_new.db", "loda.db" );
  programs.clear();
  Log::get().info( "Saved database with " + std::to_string( program_count ) + " programs" );
  dirty = false;
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
  Interpreter i( settings );
  while ( r.next( p ) )
  {
    std::cout << i.eval( p ) << std::endl;
  }
}
