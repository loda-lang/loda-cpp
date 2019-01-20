#pragma once

#include "number.hpp"
#include "finder.hpp"
#include "util.hpp"

#include <unordered_map>

class OeisSequence: public Sequence
{
public:

  OeisSequence( number_t id = 0 )
      : id( id )
  {
  }

  OeisSequence( number_t id, const std::string& name, const Sequence& s, const Sequence& full )
      : Sequence( s ),
        id( id ),
        name( name ),
        full( full )
  {
  }

  std::string id_str( const std::string& prefix = "A" ) const;

  number_t id;
  std::string name;
  Sequence full;

  friend std::ostream& operator<<( std::ostream& out, const OeisSequence& s );

  std::string to_string() const;

};

class Oeis: public Scorer
{
public:

  static size_t MAX_NUM_TERMS;

  Oeis( const Settings& settings );

  virtual ~Oeis()
  {
  }

  void load();

  virtual number_t score( const Sequence& s ) override;

  number_t findSequence( const Program& p ) const;

  void dumpProgram( number_t id, Program p, const std::string& file ) const;

  struct Hasher
  {
    std::size_t operator()( const Sequence& s ) const
    {
      std::size_t seed = s.size();
      for ( auto& i : s )
      {
        seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      return seed;
    }
  };

  const Settings& settings;
  std::vector<OeisSequence> sequences;
  std::unordered_map<Sequence, std::vector<number_t>, Hasher> ids;
  size_t total_count_;
  bool search_linear_;

};
