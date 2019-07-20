#pragma once

#include "number.hpp"
#include "program.hpp"

class Matcher
{
public:

  using seq_programs_t = std::vector<std::pair<number_t,Program>>;

  virtual ~Matcher()
  {
  }

  virtual void insert( const Sequence& norm_seq, number_t id ) = 0;

  virtual void remove( const Sequence& norm_seq, number_t id ) = 0;

  virtual void match( const Program& p, const Sequence& norm_seq, seq_programs_t& result ) const = 0;

protected:

  SequenceToIdsMap ids;

};

class DirectMatcher: public Matcher
{
public:

  virtual ~DirectMatcher()
  {
  }

  virtual void insert( const Sequence& norm_seq, number_t id ) override;

  virtual void remove( const Sequence& norm_seq, number_t id ) override;

  virtual void match( const Program& p, const Sequence& norm_seq, seq_programs_t& result ) const override;

};

class LinearMatcher: public Matcher
{
public:

  virtual ~LinearMatcher()
  {
  }

  virtual void insert( const Sequence& norm_seq, number_t id ) override;

  virtual void remove( const Sequence& norm_seq, number_t id ) override;

  virtual void match( const Program& p, const Sequence& norm_seq, seq_programs_t& result ) const override;

private:

  std::unordered_map<number_t, number_t> offsets;

};
