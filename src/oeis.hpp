#pragma once

#include "number.hpp"
#include "generator.hpp"

#include <unordered_map>

class OeisSequence: public Sequence
{
public:

  OeisSequence()
      : id( 0 )
  {
  }

  OeisSequence( number_t id, const std::string& name, const Sequence& s )
      : Sequence( s ),
        id( id ),
        name( name )
  {
  }
  number_t id;
  std::string name;

  friend std::ostream& operator<<( std::ostream& out, const OeisSequence& s )
  {
    Sequence t = s;
    out << t;
    return out;
  }

};

class Oeis: public Scorer
{
public:

  Oeis( size_t length )
      : length( length )
  {
  }

  virtual ~Oeis()
  {
  }

  virtual number_t score( const Sequence& s ) override;

  void load();

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

  size_t length;
  std::vector<OeisSequence> sequences;
  std::unordered_map<Sequence, number_t, Hasher> ids;

};
