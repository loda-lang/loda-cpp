#include "finder.hpp"

#include "number.hpp"
#include "oeis.hpp"
#include "program_util.hpp"
#include "util.hpp"

#include <fstream>

Finder::Finder( const Settings &settings )
    :
    settings( settings ),
    interpreter( settings ),
    optimizer( settings ),
    num_find_attempts( 0 )
{
  if ( settings.optimize_existing_programs )
  {
    matchers.resize( 3 );
    matchers[0].reset( new DirectMatcher( false ) );
    matchers[1].reset( new LinearMatcher( false ) );
    matchers[2].reset( new LinearMatcher2( false ) );
  }
  else
  {
    matchers.resize( 5 );
    matchers[0].reset( new DirectMatcher( true ) );
    matchers[1].reset( new LinearMatcher( true ) );
    matchers[2].reset( new LinearMatcher2( true ) );
    matchers[3].reset( new PolynomialMatcher( true ) );
    matchers[4].reset( new DeltaMatcher( true ) );
  }
  matcher_stats.resize( matchers.size() );
  for ( auto &s : matcher_stats )
  {
    s.candidates = 0;
    s.successes = 0;
    s.false_positives = 0;
    s.errors = 0;
  }
}

void Finder::insert( const Sequence &norm_seq, size_t id )
{
  for ( auto &matcher : matchers )
  {
    matcher->insert( norm_seq, id );
  }
}

void Finder::remove( const Sequence &norm_seq, size_t id )
{
  for ( auto &matcher : matchers )
  {
    matcher->remove( norm_seq, id );
  }
}

Matcher::seq_programs_t Finder::findSequence( const Program &p, Sequence &norm_seq,
    const std::vector<OeisSequence> &sequences )
{
  // update memory usage info
  if ( num_find_attempts++ % 1000 == 0 )
  {
    bool has_memory = settings.hasMemory();
    for ( auto &matcher : matchers )
    {
      matcher->has_memory = has_memory;
    }
  }

  // determine largest memory cell to check
  int64_t max_index = 20; // magic number
  number_t largest_used_cell;
  tmp_used_cells.clear();
  if ( optimizer.getUsedMemoryCells( p, tmp_used_cells, largest_used_cell ) && largest_used_cell <= 100 )
  {
    max_index = largest_used_cell;
  }

  // interpret program
  tmp_seqs.resize( std::max<size_t>( 2, max_index + 1 ) );
  Matcher::seq_programs_t result;
  try
  {
    interpreter.eval( p, tmp_seqs );
    norm_seq = tmp_seqs[1];
  }
  catch ( const std::exception& )
  {
    return result;
  }
  Program p2 = p;
  p2.push_back( Operation::Type::MOV, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::DIRECT, 0 );
  for ( size_t i = 0; i < tmp_seqs.size(); i++ )
  {
    if ( settings.search_linear || !tmp_seqs[i].is_linear( settings.linear_prefix ) )
    {
      if ( i == Program::OUTPUT_CELL )
      {
        findAll( p, tmp_seqs[i], sequences, result );
      }
      else
      {
        p2.ops.back().source.value = i;
        findAll( p2, tmp_seqs[i], sequences, result );
      }
    }
  }
  return result;
}

void Finder::findAll( const Program &p, const Sequence &norm_seq, const std::vector<OeisSequence> &sequences,
    Matcher::seq_programs_t &result )
{
  // collect possible matches
  for ( size_t i = 0; i < matchers.size(); i++ )
  {
    tmp_result.clear();
    matchers[i]->match( p, norm_seq, tmp_result );

    // validate the found matches
    size_t j = 0;
    for ( auto t : tmp_result )
    {
      matcher_stats[i].candidates++;
      auto &expected_full_seq = sequences.at( t.first ).full;
      try
      {
        tmp_full_seq.clear();
        interpreter.eval( t.second, tmp_full_seq, expected_full_seq.size() );
        if ( tmp_full_seq.size() != expected_full_seq.size() || tmp_full_seq != expected_full_seq )
        {
          matcher_stats[i].false_positives++;
          auto match_length = norm_seq.size();
          auto got = tmp_full_seq.subsequence( 0, match_length );
          auto exp = expected_full_seq.subsequence( 0, match_length );
          if ( got != exp )
          {
            auto id = sequences.at( t.first ).id_str();
            std::string f = "programs/debug/matcher/" + id + ".asm";
            ensureDir( f );
            std::ofstream o1( f );
            ProgramUtil::print( p, o1 );
            std::ofstream o2(
                "programs/debug/matcher/" + id + "-" + matchers[i]->getName() + "-" + std::to_string( j ) + ".asm" );
            ProgramUtil::print( t.second, o2 );
            Log::get().error( matchers[i]->getName() + " matcher generates wrong program for " + id );
            Log::get().error( " -  expected: " + exp.to_string() );
            Log::get().error( " -       got: " + got.to_string() );
            Log::get().error( " - generated: " + norm_seq.to_string(), true );
          }
        }
        else
        {
          // successful match!
          result.push_back( t );
          matcher_stats[i].successes++;
        }
      }
      catch ( const std::exception& )
      {
        matcher_stats[i].errors++;
      }
      j++;
    }
  }
}

void Finder::publishMetrics()
{
  for ( size_t i = 0; i < matchers.size(); i++ )
  {
    tmp_matcher_labels["matcher"] = matchers[i]->getName();
    tmp_matcher_labels["type"] = "candidate";
    Metrics::get().write( "matches", tmp_matcher_labels, matcher_stats[i].candidates );
    tmp_matcher_labels["type"] = "success";
    Metrics::get().write( "matches", tmp_matcher_labels, matcher_stats[i].successes );
    tmp_matcher_labels["type"] = "false_positive";
    Metrics::get().write( "matches", tmp_matcher_labels, matcher_stats[i].false_positives );
    tmp_matcher_labels["type"] = "error";
    Metrics::get().write( "matches", tmp_matcher_labels, matcher_stats[i].errors );
    matcher_stats[i].candidates = 0;
    matcher_stats[i].successes = 0;
    matcher_stats[i].false_positives = 0;
    matcher_stats[i].errors = 0;
  }
}
