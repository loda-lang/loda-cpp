#include "oeis_list.hpp"

#include "oeis_sequence.hpp"
#include "util.hpp"

#include <fstream>

void OeisList::loadList( const std::string& path, std::unordered_set<size_t>& list )
{
  Log::get().debug( "Loading list " + path );
  std::ifstream names( path );
  if ( !names.good() )
  {
    Log::get().warn( "Sequence list not found: " + path );
  }
  std::string line, id;
  list.clear();
  while ( std::getline( names, line ) )
  {
    if ( line.empty() || line[0] == '#' )
    {
      continue;
    }
    if ( line[0] != 'A' )
    {
      Log::get().error( "Error parsing OEIS sequence ID: " + line, true );
    }
    id = "";
    for ( char ch : line )
    {
      if ( ch == ':' || ch == ';' || ch == ' ' || ch == '\t' || ch == '\n' )
      {
        break;
      }
      id += ch;
    }
    list.insert( OeisSequence( id ).id );
  }
  Log::get().debug( "Finished loading of list " + path + " with " + std::to_string( list.size() ) + " entries" );
}

void OeisList::loadMap( const std::string& path, std::unordered_map<size_t, size_t>& map )
{
  Log::get().debug( "Loading map " + path );
  std::ifstream names( path );
  if ( !names.good() )
  {
    Log::get().warn( "Sequence list not found: " + path );
  }
  std::string line, id, value;
  bool is_value;
  map.clear();
  while ( std::getline( names, line ) )
  {
    if ( line.empty() || line[0] == '#' )
    {
      continue;
    }
    if ( line[0] != 'A' )
    {
      Log::get().error( "Error parsing OEIS sequence ID: " + line, true );
    }
    id = "";
    value = "";
    is_value = false;
    for ( char ch : line )
    {
      if ( ch == ':' || ch == ';' || ch == ',' || ch == ' ' || ch == '\t' )
      {
        is_value = true;
        continue;
      }
      if ( is_value )
      {
        value += ch;
      }
      else
      {
        id += ch;
      }
    }
    if ( id.empty() || value.empty() )
    {
      Log::get().error( "Error parsing line: " + line, true );
    }
    map[OeisSequence( id ).id] = std::stoll( value );
  }
  Log::get().debug( "Finished loading of map " + path + " with " + std::to_string( map.size() ) + " entries" );
}
