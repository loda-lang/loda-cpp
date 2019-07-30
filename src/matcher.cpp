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

// --- Linear Matcher --------------------------------------------------

Polynom LinearMatcher::reduce( Sequence& seq )
{
  int64_t offset, slope;
  auto norm_seq = seq;
  offset = seq.min();
  if ( offset > 0 )
  {
    seq.sub( offset );
  }
  slope = -1;
  for ( size_t i = 0; i < seq.size(); ++i )
  {
    int64_t new_slope = (i == 0) ? -1 : seq[i] / i;
    slope = (slope == -1) ? new_slope : (new_slope < slope ? new_slope : slope);
  }
  if ( slope > 0 )
  {
    for ( size_t i = 0; i < seq.size(); ++i )
    {
      seq[i] = seq[i] - (slope * i);
    }
  }
  Polynom result;
  result.push_back( offset );
  result.push_back( slope );
//  std::cout << "Input  " + norm_seq.to_string() << std::endl;
//  std::cout << "Output " + seq.to_string() + " with slope " + std::to_string(slope) + " and offset " + std::to_string(offset) << std::endl;
  return result;
}

void LinearMatcher::insert( const Sequence& norm_seq, number_t id )
{
//  std::cout << "Adding sequence " << id << std::endl;
  Sequence seq = norm_seq;
  polynoms[id] = reduce( seq );
  ids[seq].push_back( id ); // must be after reduce!
}

void LinearMatcher::remove( const Sequence& norm_seq, number_t id )
{
  Sequence seq = norm_seq;
  reduce( seq );
  ids.remove( seq, id );
  polynoms.erase( id );
}

void LinearMatcher::match( const Program& p, const Sequence& norm_seq, seq_programs_t& result ) const
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
      if ( optimizer.addPostLinear( copy, diff ) )
      {
//        std::cout << "Injected stuff " << std::endl;
        result.push_back( std::pair<number_t, Program>( id, copy ) );
      }
    }
  }
}
