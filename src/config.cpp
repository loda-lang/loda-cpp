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

std::string get_file_as_string( const std::string& filename )
{
  std::ifstream in( filename );
  std::string str;
  if ( in.good() )
  {
    std::string tmp;
    while ( getline( in, tmp ) )
    {
      str += tmp;
    }
    in.close();
  }
  if ( str.empty() )
  {
    Log::get().error( "Error loading " + filename, true );
  }
  return str;
}

std::vector<Generator::Config> loadGeneratorConfigs( const std::string& miner, jute::jValue &gens,
    const std::unordered_set<std::string>& names )
{
  std::vector<Generator::Config> generators;
  for ( int i = 0; i < gens.size(); i++ )
  {
    auto g = gens[i];
    auto name = g["name"].as_string();
    if ( names.find( name ) == names.end() )
    {
      continue;
    }
    Generator::Config c;
    c.version = get_jint( g, "version", 1 );
    c.miner = miner;
    c.length = get_jint( g, "length", 20 );
    c.max_constant = get_jint( g, "maxConstant", 4 );
    c.max_index = get_jint( g, "maxIndex", 4 );
    c.loops = get_jbool( g, "loops", true );
    c.calls = get_jbool( g, "calls", true );
    c.indirect_access = get_jbool( g, "indirectAccess", false );
    switch ( g["template"].get_type() )
    {
    case jute::jType::JSTRING:
    {
      c.program_template = g["template"].as_string();
      generators.push_back( c );
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
          generators.push_back( c );
        }
      }
      break;
    }
    default:
    {
      generators.push_back( c );
      break;
    }
    }
  }
  return generators;
}

Miner::Config ConfigLoader::load( const Settings& settings )
{
  Log::get().debug( "Loading miner config \"" + settings.miner + "\" from " + settings.loda_config );
  Miner::Config config;

  auto str = get_file_as_string( settings.loda_config );
  auto spec = jute::parser::parse( str );
  auto miners = spec["miners"];

  int index = -1;
  if ( !settings.miner.empty() && std::find_if( settings.miner.begin(), settings.miner.end(), [](unsigned char c)
  { return !std::isdigit(c);} ) == settings.miner.end() )
  {
    index = stoi( settings.miner ) % miners.size();
  }

  bool found = false;
  for ( int i = 0; i < miners.size(); i++ )
  {
    auto m = miners[i];
    auto name = m["name"].as_string();
    if ( name == settings.miner || i == index )
    {
      config.name = name;
      auto overwrite_mode = m["overwrite"].as_string();
      if ( overwrite_mode == "none" )
      {
        config.overwrite_mode = OverwriteMode::NONE;
      }
      else if ( overwrite_mode == "all" )
      {
        config.overwrite_mode = OverwriteMode::ALL;
      }
      else if ( overwrite_mode == "auto" )
      {
        config.overwrite_mode = OverwriteMode::AUTO;
      }
      else
      {
        throw std::runtime_error( "Unknown overwrite mode: " + overwrite_mode );
      }

      // load matcher configs
      bool backoff = get_jbool( m, "backoff", true );
      auto matchers = m["matchers"];
      for ( int j = 0; j < matchers.size(); j++ )
      {
        Matcher::Config mc;
        mc.backoff = backoff;
        mc.type = matchers[j].as_string();
        config.matchers.push_back( mc );
      }

      // load generator configs
      auto gen_names = m["generators"];
      std::unordered_set<std::string> names;
      for ( int j = 0; j < gen_names.size(); j++ )
      {
        names.insert( gen_names[j].as_string() );
      }
      auto gens = spec["generators"];
      config.generators = loadGeneratorConfigs( name, gens, names );

      // done
      found = true;
      break;
    }
  }
  if ( !found )
  {
    Log::get().error( "Miner config not found: " + settings.miner, true );
  }
  Log::get().debug(
      "Finished loading miner config \"" + settings.miner + "\" from " + settings.loda_config + " with "
          + std::to_string( config.generators.size() ) + " generators" );
  return config;
}
