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

void LinearMatcher::reduce( Sequence& seq, int64_t& slope, int64_t& offset )
{
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
//  std::cout << "Input  " + norm_seq.to_string() << std::endl;
//  std::cout << "Output " + seq.to_string() + " with slope " + std::to_string(slope) + " and offset " + std::to_string(offset) << std::endl;
}

void LinearMatcher::insert( const Sequence& norm_seq, number_t id )
{
//  std::cout << "Adding sequence " << id << std::endl;
  Sequence seq = norm_seq;
  int64_t slope;
  int64_t offset;
  reduce( seq, slope, offset );
  ids[seq].push_back( id );
  offsets[id] = offset;
  slopes[id] = slope;
}

void LinearMatcher::remove( const Sequence& norm_seq, number_t id )
{
  Sequence seq = norm_seq;
  int64_t slope;
  int64_t offset;
  reduce( seq, slope, offset );
  ids.remove( seq, id );
  offsets.erase( id );
  slopes.erase( id );
}

void LinearMatcher::match( const Program& p, const Sequence& norm_seq, seq_programs_t& result ) const
{
//  std::cout << "Matching sequence " << norm_seq.to_string() << std::endl;
  Sequence seq = norm_seq;
  int64_t slope;
  int64_t offset;
  reduce( seq, slope, offset );
  auto it = ids.find( seq );
  if ( it != ids.end() )
  {
    for ( auto id : it->second )
    {
//      std::cout << "Matched sequence " << id << std::endl;
      int64_t target_offset = offsets.at( id );
      int64_t delta_offset = target_offset - offset;

      int64_t target_slope = slopes.at( id );
      int64_t delta_slope = target_slope - slope;

      Settings s;
      Optimizer optimizer( s );
      Program copy = p;
      if ( optimizer.addPostLinear( copy, delta_slope, delta_offset ) )
      {
//        std::cout << "Inject slope " << delta_slope << " and offset " << delta_offset << std::endl;
        result.push_back( std::pair<number_t, Program>( id, copy ) );
      }
    }
  }
}
