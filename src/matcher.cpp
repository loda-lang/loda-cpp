#include "matcher.hpp"

#include "optimizer.hpp"
#include "reducer.hpp"
#include "semantics.hpp"

// --- AbstractMatcher --------------------------------------------------------

template<class T>
void AbstractMatcher<T>::insert( const Sequence &norm_seq, size_t id )
{
  auto reduced = reduce( norm_seq );
  data[id] = reduced.second;
  ids[reduced.first].push_back( id );
}

template<class T>
void AbstractMatcher<T>::remove( const Sequence &norm_seq, size_t id )
{
  auto reduced = reduce( norm_seq );
  ids.remove( reduced.first, id );
  data.erase( id );
}

template<class T>
void AbstractMatcher<T>::match( const Program &p, const Sequence &norm_seq, seq_programs_t &result ) const
{
  if ( !shouldMatchSequence( norm_seq ) )
  {
    return;
  }
  auto reduced = reduce( norm_seq );
  if ( !shouldMatchSequence( reduced.first ) )
  {
    return;
  }
  auto it = ids.find( reduced.first );
  if ( it != ids.end() )
  {
    for ( auto id : it->second )
    {
      Program copy = p;
      if ( extend( copy, data.at( id ), reduced.second ) )
      {
        result.push_back( std::pair<size_t, Program>( id, copy ) );
      }
    }
  }
}

template<class T>
bool AbstractMatcher<T>::shouldMatchSequence( const Sequence &seq ) const
{
  if ( backoff && match_attempts.find( seq ) != match_attempts.end() )
  {
    // Log::get().debug( "Back off matching of already matched sequence " + seq.to_string() );
    return false;
  }
  if ( has_memory )
  {
    match_attempts.insert( seq );
  }
  return true;
}

// --- Direct Matcher ---------------------------------------------------------

std::pair<Sequence, int> DirectMatcher::reduce( const Sequence &seq ) const
{
  std::pair<Sequence, int> result;
  result.first = seq;
  result.second = 0;
  return result;
}

bool DirectMatcher::extend( Program &p, int base, int gen ) const
{
  return true;
}

// --- Linear Matcher ---------------------------------------------------------

std::pair<Sequence, line_t> LinearMatcher::reduce( const Sequence &seq ) const
{
  std::pair<Sequence, line_t> result;
  result.first = seq;
  result.second.offset = Reducer::truncate( result.first );
  result.second.factor = Reducer::shrink( result.first );
  return result;
}

bool LinearMatcher::extend( Program &p, line_t base, line_t gen ) const
{
  return Extender::linear1( p, gen, base );
}

std::pair<Sequence, line_t> LinearMatcher2::reduce( const Sequence &seq ) const
{
  std::pair<Sequence, line_t> result;
  result.first = seq;
  result.second.factor = Reducer::shrink( result.first );
  result.second.offset = Reducer::truncate( result.first );
  return result;
}

bool LinearMatcher2::extend( Program &p, line_t base, line_t gen ) const
{
  return Extender::linear2( p, gen, base );
}

// --- Polynomial Matcher -----------------------------------------------------

const int PolynomialMatcher::DEGREE = 3; // magic number

std::pair<Sequence, Polynomial> PolynomialMatcher::reduce( const Sequence &seq ) const
{
  std::pair<Sequence, Polynomial> result;
  result.first = seq;
  result.second = Reducer::polynomial( result.first, DEGREE );
  return result;
}

bool PolynomialMatcher::extend( Program &p, Polynomial base, Polynomial gen ) const
{
  auto diff = base - gen;
  return Extender::polynomial( p, diff );
}

// --- Delta Matcher ----------------------------------------------------------

const int DeltaMatcher::MAX_DELTA = 4; // magic number

std::pair<Sequence, delta_t> DeltaMatcher::reduce( const Sequence &seq ) const
{
  std::pair<Sequence, delta_t> result;
  result.first = seq;
  result.second = Reducer::delta( result.first, MAX_DELTA );
  return result;
}

bool DeltaMatcher::extend( Program &p, delta_t base, delta_t gen ) const
{
  if ( base.offset == gen.offset && base.factor == gen.factor )
  {
    return Extender::delta_it( p, base.delta - gen.delta );
  }
  else
  {
    if ( !Extender::delta_it( p, -gen.delta ) )
    {
      return false;
    }
    line_t base_line;
    base_line.offset = base.offset;
    base_line.factor = base.factor;
    line_t gen_line;
    gen_line.offset = gen.offset;
    gen_line.factor = gen.factor;
    if ( !Extender::linear1( p, gen_line, base_line ) )
    {
      return false;
    }
    if ( !Extender::delta_it( p, base.delta ) )
    {
      return false;
    }
  }
  return true;
}
