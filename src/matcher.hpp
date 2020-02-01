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

};

template<class T>
class AbstractMatcher: public Matcher
{
public:

  virtual ~AbstractMatcher()
  {
  }

  virtual void insert( const Sequence &norm_seq, number_t id ) override;

  virtual void remove( const Sequence &norm_seq, number_t id ) override;

  virtual void match( const Program &p, const Sequence &norm_seq, seq_programs_t &result ) const override;

protected:

  virtual std::pair<Sequence, T> reduce( const Sequence &seq ) const = 0;

  virtual bool extend( Program &p, T base, T gen ) const = 0;

private:

  SequenceToIdsMap ids;
  std::unordered_map<number_t, T> data;

};

class DirectMatcher: public AbstractMatcher<int>
{
public:

  virtual ~DirectMatcher()
  {
  }

protected:

  virtual std::pair<Sequence, int> reduce( const Sequence &seq ) const override;

  virtual bool extend( Program &p, int base, int gen ) const override;

};

struct line
{
  int offset;
  int factor;
};

class LinearMatcher: public AbstractMatcher<line>
{
public:

  virtual ~LinearMatcher()
  {
  }

protected:

  virtual std::pair<Sequence, line> reduce( const Sequence &seq ) const override;

  virtual bool extend( Program &p, line base, line gen ) const override;

};

class PolynomialMatcher: public AbstractMatcher<Polynomial>
{
public:

  static const int DEGREE;

  virtual ~PolynomialMatcher()
  {
  }

protected:

  virtual std::pair<Sequence, Polynomial> reduce( const Sequence &seq ) const override;

  virtual bool extend( Program &p, Polynomial base, Polynomial gen ) const override;

private:

  Polynomial reduce( Sequence &seq, int64_t exp ) const;

};

class DeltaMatcher: public AbstractMatcher<int>
{
public:

  static const int MAX_DELTA;

  virtual ~DeltaMatcher()
  {
  }

protected:

  virtual std::pair<Sequence, int> reduce( const Sequence &seq ) const override;

  virtual bool extend( Program &p, int base, int gen ) const override;

};
