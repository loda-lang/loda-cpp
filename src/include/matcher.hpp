#pragma once

#include "number.hpp"
#include "program.hpp"
#include "reducer.hpp"
#include "extender.hpp"

#include <unordered_set>

class Matcher
{
public:

  using seq_programs_t = std::vector<std::pair<size_t,Program>>;

  virtual ~Matcher()
  {
  }

  virtual void insert( const Sequence &norm_seq, size_t id ) = 0;

  virtual void remove( const Sequence &norm_seq, size_t id ) = 0;

  virtual void match( const Program &p, const Sequence &norm_seq, seq_programs_t &result ) const = 0;

  virtual const std::string& getName() const = 0;

  virtual const SequenceToIdsMap& getReducedSequences() const = 0;

  bool has_memory = true;

};

template<class T>
class AbstractMatcher: public Matcher
{
public:

  static constexpr size_t MAX_NUM_MATCHES = 10; // magic number

  AbstractMatcher( const std::string &name, bool backoff )
      :
      name( name ),
      backoff( backoff )
  {
  }

  virtual ~AbstractMatcher()
  {
  }

  virtual void insert( const Sequence &norm_seq, size_t id ) override;

  virtual void remove( const Sequence &norm_seq, size_t id ) override;

  virtual void match( const Program &p, const Sequence &norm_seq, seq_programs_t &result ) const override;

  virtual const std::string& getName() const override
  {
    return name;
  }

  virtual const SequenceToIdsMap& getReducedSequences() const override
  {
    return ids;
  }

protected:

  virtual std::pair<Sequence, T> reduce( const Sequence &seq ) const = 0;

  virtual bool extend( Program &p, T base, T gen ) const = 0;

private:

  bool shouldMatchSequence( const Sequence &seq ) const;

  std::string name;
  SequenceToIdsMap ids;
  std::unordered_map<size_t, T> data;
  mutable std::unordered_set<Sequence, SequenceHasher> match_attempts;
  bool backoff;

};

class DirectMatcher: public AbstractMatcher<int>
{
public:

  DirectMatcher( bool backoff )
      :
      AbstractMatcher( "direct", backoff )
  {
  }

  virtual ~DirectMatcher()
  {
  }

protected:

  virtual std::pair<Sequence, int> reduce( const Sequence &seq ) const override;

  virtual bool extend( Program &p, int base, int gen ) const override;

};

class LinearMatcher: public AbstractMatcher<line_t>
{
public:

  LinearMatcher( bool backoff )
      :
      AbstractMatcher( "linear1", backoff )
  {
  }

  virtual ~LinearMatcher()
  {
  }

protected:

  virtual std::pair<Sequence, line_t> reduce( const Sequence &seq ) const override;

  virtual bool extend( Program &p, line_t base, line_t gen ) const override;

};

class LinearMatcher2: public AbstractMatcher<line_t>
{
public:

  LinearMatcher2( bool backoff )
      :
      AbstractMatcher( "linear2", backoff )
  {
  }

  virtual ~LinearMatcher2()
  {
  }

protected:

  virtual std::pair<Sequence, line_t> reduce( const Sequence &seq ) const override;

  virtual bool extend( Program &p, line_t base, line_t gen ) const override;

};

class PolynomialMatcher: public AbstractMatcher<Polynomial>
{
public:

  static const int DEGREE;

  PolynomialMatcher( bool backoff )
      :
      AbstractMatcher( "polynomial", backoff )
  {
  }

  virtual ~PolynomialMatcher()
  {
  }

protected:

  virtual std::pair<Sequence, Polynomial> reduce( const Sequence &seq ) const override;

  virtual bool extend( Program &p, Polynomial base, Polynomial gen ) const override;

private:

  Polynomial reduce( Sequence &seq, int64_t exp ) const;

};

class DeltaMatcher: public AbstractMatcher<delta_t>
{
public:

  static const int MAX_DELTA;

  DeltaMatcher( bool backoff )
      :
      AbstractMatcher( "delta", backoff )
  {
  }

  virtual ~DeltaMatcher()
  {
  }

protected:

  virtual std::pair<Sequence, delta_t> reduce( const Sequence &seq ) const override;

  virtual bool extend( Program &p, delta_t base, delta_t gen ) const override;

};
