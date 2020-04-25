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

  virtual const std::string& getName() const = 0;

  virtual size_t getDomainSize() const = 0;

};

template<class T>
class AbstractMatcher: public Matcher
{
public:

  AbstractMatcher( const std::string &name )
      :
      name( name )
  {
  }

  virtual ~AbstractMatcher()
  {
  }

  virtual void insert( const Sequence &norm_seq, number_t id ) override;

  virtual void remove( const Sequence &norm_seq, number_t id ) override;

  virtual void match( const Program &p, const Sequence &norm_seq, seq_programs_t &result ) const override;

  virtual const std::string& getName() const override
  {
    return name;
  }

  virtual size_t getDomainSize() const override
  {
    return ids.size();
  }

protected:

  virtual std::pair<Sequence, T> reduce( const Sequence &seq ) const = 0;

  virtual bool extend( Program &p, T base, T gen ) const = 0;

private:

  std::string name;
  SequenceToIdsMap ids;
  std::unordered_map<number_t, T> data;

};

class DirectMatcher: public AbstractMatcher<int>
{
public:

  DirectMatcher()
      :
      AbstractMatcher( "direct" )
  {
  }

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

  LinearMatcher()
      :
      AbstractMatcher( "linear1" )
  {
  }

  virtual ~LinearMatcher()
  {
  }

protected:

  virtual std::pair<Sequence, line> reduce( const Sequence &seq ) const override;

  virtual bool extend( Program &p, line base, line gen ) const override;

};

class LinearMatcher2: public AbstractMatcher<line>
{
public:

  LinearMatcher2()
      :
      AbstractMatcher( "linear2" )
  {
  }

  virtual ~LinearMatcher2()
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

  PolynomialMatcher()
      :
      AbstractMatcher( "polynomial" )
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

struct delta_t
{
  int delta;
  int factor;
};

class DeltaMatcher: public AbstractMatcher<delta_t>
{
public:

  static const int MAX_DELTA;

  DeltaMatcher()
      :
      AbstractMatcher( "delta" )
  {
  }

  virtual ~DeltaMatcher()
  {
  }

protected:

  virtual std::pair<Sequence, delta_t> reduce( const Sequence &seq ) const override;

  virtual bool extend( Program &p, delta_t base, delta_t gen ) const override;

};
