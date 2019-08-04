#include "matcher.hpp"

#include "optimizer.hpp"

// --- Direct Matcher --------------------------------------------------

void DirectMatcher::insert( const Sequence& norm_seq, number_t id )
{
  ids[norm_seq].push_back( id );
}

void DirectMatcher::remove( const Sequence& norm_seq, number_t id )
{
  ids.remove( norm_seq, id );
}

void DirectMatcher::match( const Program& p, const Sequence& norm_seq, seq_programs_t& result ) const
{
  auto it = ids.find( norm_seq );
  if ( it != ids.end() )
  {
    for ( auto id : it->second )
    {
      result.push_back( std::pair<number_t, Program>( id, p ) );
    }
  }
}

// --- Polynomial Matcher --------------------------------------------------

const int PolynomialMatcher::DEGREE = 1;

Polynom PolynomialMatcher::reduce( Sequence& seq )
{
  Polynom polynom( DEGREE );
//  auto input_seq = seq;
//  std::cout << "Input  " + input_seq.to_string() << std::endl;
  for ( int exp = 0; exp <= DEGREE; exp++ )
  {
    int64_t factor = -1;
    for ( size_t x = 0; x < seq.size(); x++ )
    {
      int64_t x_exp = 1;
      for ( int e = 0; e < exp; e++ )
      {
        x_exp *= x;
      }
      int64_t new_factor = (x_exp == 0) ? -1 : seq[x] / x_exp;
      factor = (factor == -1) ? new_factor : (new_factor < factor ? new_factor : factor);
      if ( factor == 0 ) break;
    }
    if ( factor > 0 )
    {
      for ( size_t x = 0; x < seq.size(); x++ )
      {
        int64_t x_exp = 1;
        for ( int e = 0; e < exp; e++ )
        {
          x_exp *= x;
        }
        seq[x] = seq[x] - (factor * x_exp);
      }
    }
    polynom[exp] = factor;
  }
//  std::cout << "Output " + seq.to_string() + " with polynom " + polynom.to_string() << std::endl;
  return polynom;
}

void PolynomialMatcher::insert( const Sequence& norm_seq, number_t id )
{
//  std::cout << "Adding sequence " << id << std::endl;
  Sequence seq = norm_seq;
  polynoms[id] = reduce( seq );
  ids[seq].push_back( id ); // must be after reduce!
}

void PolynomialMatcher::remove( const Sequence& norm_seq, number_t id )
{
  Sequence seq = norm_seq;
  reduce( seq );
  ids.remove( seq, id );
  polynoms.erase( id );
}

void PolynomialMatcher::match( const Program& p, const Sequence& norm_seq, seq_programs_t& result ) const
{
//  std::cout << "Matching sequence " << norm_seq.to_string() << std::endl;
  Sequence seq = norm_seq;
  Polynom pol = reduce( seq );
  auto it = ids.find( seq );
  if ( it != ids.end() )
  {
    for ( auto id : it->second )
    {
//      std::cout << "Matched sequence " << id << std::endl;
      auto diff = polynoms.at( id ) - pol;

      Settings s;
      Optimizer optimizer( s );
      Program copy = p;
      if ( optimizer.addPostPolynomial( copy, diff ) )
      {
//        std::cout << "Injected stuff " << std::endl;
        result.push_back( std::pair<number_t, Program>( id, copy ) );
      }
    }
  }
}
