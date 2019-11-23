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

  virtual void insert( const Sequence &norm_seq, number_t id ) = 0;

  virtual void remove( const Sequence &norm_seq, number_t id ) = 0;

  virtual void match( const Program &p, const Sequence &norm_seq, seq_programs_t &result ) const = 0;

protected:

  SequenceToIdsMap ids;

};

class DirectMatcher: public Matcher
{
public:

  virtual ~DirectMatcher()
  {
  }

  virtual void insert( const Sequence &norm_seq, number_t id ) override;

  virtual void remove( const Sequence &norm_seq, number_t id ) override;

  virtual void match( const Program &p, const Sequence &norm_seq, seq_programs_t &result ) const override;

};

class PolynomialMatcher: public Matcher
{
public:

  static const int DEGREE;

  virtual ~PolynomialMatcher()
  {
  }

  virtual void insert( const Sequence &norm_seq, number_t id ) override;

  virtual void remove( const Sequence &norm_seq, number_t id ) override;

  virtual void match( const Program &p, const Sequence &norm_seq, seq_programs_t &result ) const override;

private:

  Polynomial reduce( Sequence &seq, int64_t exp ) const;

  std::unordered_map<number_t, Polynomial> polynoms;

};

class DeltaMatcher: public Matcher
{
public:

  static const int MAX_DELTA;

  virtual ~DeltaMatcher()
  {
  }

  virtual void insert( const Sequence &norm_seq, number_t id ) override;

  virtual void remove( const Sequence &norm_seq, number_t id ) override;

  virtual void match( const Program &p, const Sequence &norm_seq, seq_programs_t &result ) const override;

private:

  std::pair<Sequence, int64_t> reduce( const Sequence &seq ) const;

  std::unordered_map<number_t, int64_t> deltas;

};
