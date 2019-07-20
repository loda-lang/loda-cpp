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

void LinearMatcher::insert( const Sequence& norm_seq, number_t id )
{
  number_t offset = norm_seq.min();
  if ( offset > 0 )
  {
    auto reduced_seq = norm_seq;
    reduced_seq.sub( offset );
    ids[reduced_seq].push_back( id );
    offsets[id] = offset;
  }
}

void LinearMatcher::remove( const Sequence& norm_seq, number_t id )
{
  number_t offset = norm_seq.min();
  if ( offset > 0 )
  {
    auto reduced_seq = norm_seq;
    reduced_seq.sub( offset );
    ids.remove( reduced_seq, id );
    offsets.erase( id );
  }
}

void LinearMatcher::match( const Program& p, const Sequence& norm_seq, seq_programs_t& result ) const
{
  int64_t offset = norm_seq.min();
  auto reduced_seq = norm_seq;
  reduced_seq.sub( offset );
  auto it = ids.find( reduced_seq );
  if ( it != ids.end() )
  {
    for ( auto id : it->second )
    {
      int64_t base_offset = offsets.at( id );
      int64_t delta = base_offset - offset;
      Settings s;
      Optimizer optimizer( s );
      Program copy = p;
      optimizer.addPostLinear( copy, 0, delta );
      result.push_back( std::pair<number_t, Program>( id, copy ) );
    }
  }
}
