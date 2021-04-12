#include "config.hpp"

#include "jute.h"

int64_t get_jint( jute::jValue &v, const std::string &key, int64_t def )
{
  if ( v[key].get_type() == jute::jType::JNUMBER )
  {
    return v[key].as_int();
  }
  return def;
}

bool get_jbool( jute::jValue &v, const std::string &key, bool def )
{
  if ( v[key].get_type() == jute::jType::JBOOLEAN )
  {
    return v[key].as_bool();
  }
  return def;
}

LODAConfig::LODAConfig( const Settings& settings )
{
  std::ifstream in( settings.loda_config );
  std::string str = "";
  std::string tmp;
  while ( getline( in, tmp ) )
  {
    str += tmp;
  }
  auto spec = jute::parser::parse( str );
  auto generators = spec["generators"];
  auto gens = settings.optimize_existing_programs ? generators["update"] : generators["new"];
  for ( int i = 0; i < gens.size(); i++ )
  {
    auto g = gens[i];
    Generator::Config c;
    c.version = get_jint( g, "version", 1 );
    c.replicas = get_jint( g, "replicas", 1 );
    if ( c.version == 1 )
    {
      c.length = get_jint( g, "length", 20 );
      c.max_constant = get_jint( g, "maxConstant", 4 );
      c.max_index = get_jint( g, "maxIndex", 4 );
      c.loops = get_jbool( g, "loops", true );
      c.calls = get_jbool( g, "calls", true );
      c.indirect_access = get_jbool( g, "indirectAccess", false );
    }
    switch ( g["template"].get_type() )
    {
    case jute::jType::JSTRING:
    {
      c.program_template = g["template"].as_string();
      generator_configs.push_back( c );
      break;
    }
    case jute::jType::JARRAY:
    {
      auto a = g["template"];
      for ( int i = 0; i < a.size(); i++ )
      {
        if ( a[i].get_type() == jute::jType::JSTRING )
        {
          c.program_template = a[i].as_string();
          generator_configs.push_back( c );
        }
      }
      break;
    }
    default:
    {
      generator_configs.push_back( c );
      break;
    }
    }
  }
  Log::get().debug( "Loaded " + std::to_string( generator_configs.size() ) + " generator configurations" );
}
