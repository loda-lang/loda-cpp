#pragma once

#include <algorithm>
#include <limits>
#include <random>
#include <stdlib.h>

#define USE_BIG_NUMBER false
#define FORCE_BIG_NUMBER false

class BigNumber;

class Number
{
public:

  static const Number ZERO;
  static const Number ONE;
  static const Number MIN;
  static const Number MAX;
  static const Number INF;

  Number();

  Number( const Number& n );

  Number( int64_t value );

  Number( const std::string& s );

  ~Number();

  Number& operator=( const Number& n );

  bool operator==( const Number& n ) const;

  bool operator!=( const Number& n ) const;

  bool operator<( const Number& n ) const;

  Number& negate();

  Number& operator+=( const Number& n );

  Number& operator*=( const Number& n );

  Number& operator/=( const Number& n );

  Number& operator%=( const Number& n );

  int64_t asInt() const;

  int64_t getNumUsedWords() const;

  std::size_t hash() const;

  friend std::ostream& operator<<( std::ostream &out, const Number &n );

  std::string to_string() const;

  static void readIntString( std::istream& in, std::string& out );

private:

  // TODO: avoid this friend class
  friend class OeisSequence;

  static Number infinity();

  static Number minMax( bool is_max );

  bool checkInfArgs( const Number& n );

  void checkInfBig();

  void convertToBig();

  int64_t value;
  BigNumber* big;

};
