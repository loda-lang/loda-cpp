#include "oeis.hpp"

#include "interpreter.hpp"
#include "number.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "util.hpp"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <fstream>
#include <sstream>

size_t Oeis::MAX_NUM_TERMS = 250;

std::ostream& operator<<( std::ostream &out, const OeisSequence &s )
{
  out << s.id_str() << ": " << s.name;
  return out;
}

std::string OeisSequence::to_string() const
{
  std::stringstream ss;
  ss << (*this);
  return ss.str();
}

std::string OeisSequence::id_str( const std::string &prefix ) const
{
  std::stringstream s;
  s << prefix << std::setw( 6 ) << std::setfill( '0' ) << id;
  return s.str();
}

void throwParseError( const std::string &line )
{
  Log::get().error( "error parsing OEIS line: " + line, true );
}

std::string getHome()
{
  return std::string( std::getenv( "HOME" ) ) + "/.loda/oeis/";
}

std::string getOeisFile( const OeisSequence &seq )
{
  return "programs/oeis/" + seq.id_str() + ".asm";
}

Oeis::Oeis( const Settings &settings )
    :
    settings( settings ),
    interpreter( settings ),
    optimizer( settings ),
    total_count_( 0 )
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
    s.false_positives = 0;
    s.errors = 0;
  }
}

bool isCloseToOverflow( number_t n )
{
  return n > (NUM_INF / 10000);
}

void Oeis::load()
{
  // load sequence data
  Log::get().info( "Loading sequence data from the OEIS" );
  std::ifstream stripped( getHome() + "stripped" );
  if ( !stripped.good() )
  {
    Log::get().error( "OEIS data not found: run get_oeis.sh to download it", true );
  }
  std::string line;
  size_t pos;
  number_t id, num;
  Sequence full_sequence;
  Sequence norm_sequence;
  Sequence big_sequence;
  size_t loaded_count = 0;
  while ( std::getline( stripped, line ) )
  {
    if ( line.empty() || line[0] == '#' )
    {
      continue;
    }
    if ( line[0] != 'A' )
    {
      throwParseError( line );
    }
    ++total_count_;
    pos = 1;
    id = 0;
    for ( pos = 1; pos < line.length() && line[pos] >= '0' && line[pos] <= '9'; ++pos )
    {
      id = (10 * id) + (line[pos] - '0');
    }
    if ( pos >= line.length() || line[pos] != ' ' || id == 0 )
    {
      throwParseError( line );
    }
    ++pos;
    if ( pos >= line.length() || line[pos] != ',' )
    {
      throwParseError( line );
    }
    ++pos;
    num = 0;
    full_sequence.clear();
    while ( pos < line.length() )
    {
      if ( line[pos] == ',' )
      {
        full_sequence.push_back( num );
        num = 0;
      }
      else if ( line[pos] >= '0' && line[pos] <= '9' )
      {
        if ( isCloseToOverflow( num ) )
        {
          break;
        }
        num = (10 * num) + (line[pos] - '0');
      }
      else if ( line[pos] == '-' )
      {
        full_sequence.clear();
        break;
      }
      else
      {
        throwParseError( line );
      }
      ++pos;
    }

    // check minimum number of terms
    if ( full_sequence.size() < settings.num_terms )
    {
      continue;
    }

    // normalized sequence
    norm_sequence = Sequence(
        std::vector<number_t>( full_sequence.begin(), full_sequence.begin() + settings.num_terms ) );

    // big sequence
    big_sequence.clear();
    std::stringstream big_path;
    big_path << getHome() << "b/b" << std::setw( 6 ) << std::setfill( '0' ) << id << ".txt";
    std::ifstream big_file( big_path.str() );
    if ( big_file.good() )
    {
      std::string l;
      int64_t expected_index = -1, index = 0, value = 0;
      while ( std::getline( big_file, l ) )
      {
        l.erase( l.begin(), std::find_if( l.begin(), l.end(), []( int ch )
        {
          return !std::isspace(ch);
        } ) );
        if ( l.empty() || l[0] == '#' )
        {
          continue;
        }
        std::stringstream ss( l );
        ss >> index >> value;
        if ( expected_index == -1 )
        {
          if ( index < 0 )
          {
            continue;
          }
          expected_index = index;
          if ( value != (int) full_sequence.front() )
          {
            for ( size_t i = 0; i < 5; i++ )
            {
              if ( value != (int) full_sequence[i] )
              {
                big_sequence.push_back( full_sequence[i] );
              }
              else
              {
                break;
              }
            }
          }
        }
        if ( index != expected_index )
        {
          Log::get().warn( "Unexpected index " + std::to_string( index ) + " in b-file " + big_path.str() );
          big_sequence.clear();
          break;
        }
        if ( value < 0 )
        {
          big_sequence.clear();
          break;
        }
        if ( isCloseToOverflow( (number_t) value ) )
        {
          break;
        }
        big_sequence.push_back( value );
        ++expected_index;
      }
      if ( big_sequence.size() >= full_sequence.size() )
      {
        Sequence test_sequence(
            std::vector<number_t>( big_sequence.begin(), big_sequence.begin() + full_sequence.size() ) );
        if ( test_sequence != full_sequence )
        {
          Log::get().warn( "Unexpected terms in b-file " + big_path.str() );
          big_sequence.clear();
        }
      }
      else
      {
        Log::get().debug( "Sequence in b-file too short: " + big_path.str() );
        big_sequence.clear();
      }
      if ( big_sequence.size() > MAX_NUM_TERMS )
      {
        big_sequence = Sequence( std::vector<number_t>( big_sequence.begin(), big_sequence.begin() + MAX_NUM_TERMS ) );
      }
      if ( !big_sequence.empty() )
      {
        full_sequence = big_sequence;
        Log::get().debug(
            "Loaded b-file for sequence " + std::to_string( id ) + " with " + std::to_string( big_sequence.size() )
                + " terms" );
      }
    }
    else
    {
      Log::get().debug( "b-file not found: " + big_path.str() );
    }

    // add sequence to index
    if ( id >= sequences.size() )
    {
      sequences.resize( 2 * id );
    }
    sequences[id] = OeisSequence( id, "", norm_sequence, full_sequence );

    // add sequences to matchers
    for ( auto &matcher : matchers )
    {
      matcher->insert( norm_sequence, id );
    }

    ++loaded_count;
  }

  loadNames();

  if ( !settings.optimize_existing_programs )
  {
    std::vector<number_t> seqs_to_remove;
    for ( auto &seq : sequences )
    {
      std::ifstream in( getOeisFile( seq ) );
      if ( in.good() )
      {
        seqs_to_remove.push_back( seq.id );
        in.close();
      }
    }
    if ( !seqs_to_remove.empty() )
    {
      Log::get().info(
          "Ignoring " + std::to_string( seqs_to_remove.size() )
              + " sequences because programs exist for them already" );
      for ( auto id : seqs_to_remove )
      {
        removeSequence( id );
      }
    }
  }

  Log::get().info(
      "Loaded " + std::to_string( loaded_count ) + "/" + std::to_string( total_count_ ) + " sequences from the OEIS" );
  std::stringstream buf;
  buf << "Matcher compaction ratios: ";
  for ( size_t i = 0; i < matchers.size(); i++ )
  {
    if ( i > 0 ) buf << ", ";
    double ratio = 100.0 * (double) matchers[i]->getReducedSequences().size() / (double) loaded_count;
    buf << matchers[i]->getName() << ": " << std::setprecision( 4 ) << ratio << "%";
  }
  Log::get().info( buf.str() );

}

void Oeis::loadNames()
{
  Log::get().debug( "Loading sequence names from the OEIS" );
  std::ifstream names( getHome() + "names" );
  if ( !names.good() )
  {
    Log::get().error( "OEIS data not found: run get_oeis.sh to download it", true );
  }
  std::string line;
  size_t pos;
  number_t id;
  while ( std::getline( names, line ) )
  {
    if ( line.empty() || line[0] == '#' )
    {
      continue;
    }
    if ( line[0] != 'A' )
    {
      throwParseError( line );
    }
    pos = 1;
    id = 0;
    for ( pos = 1; pos < line.length() && line[pos] >= '0' && line[pos] <= '9'; ++pos )
    {
      id = (10 * id) + (line[pos] - '0');
    }
    if ( pos >= line.length() || line[pos] != ' ' || id == 0 )
    {
      throwParseError( line );
    }
    ++pos;
    if ( id < sequences.size() && sequences[id].id == id )
    {
      sequences[id].name = line.substr( pos );
      if ( Log::get().level == Log::Level::DEBUG )
      {
        std::stringstream buf;
        buf << "Loaded sequence " << sequences[id];
        Log::get().debug( buf.str() );
      }
    }
  }
}

const std::vector<OeisSequence>& Oeis::getSequences() const
{
  return sequences;
}

void Oeis::removeSequence( number_t id )
{
  if ( id >= sequences.size() )
  {
    return;
  }
  if ( sequences[id].id == id )
  {
    for ( auto &matcher : matchers )
    {
      matcher->remove( sequences[id], id );
    }
    sequences[id] = OeisSequence();
  }
}

Oeis::seq_programs_t Oeis::findSequence( const Program &p, Sequence &norm_seq ) const
{
  std::vector<Sequence> seqs;
  seqs.resize( std::max<size_t>( 2, settings.max_index + 1 ) );
  seq_programs_t result;
  try
  {
    interpreter.eval( p, seqs );
    norm_seq = seqs[1];
  }
  catch ( const std::exception& )
  {
    return result;
  }
  Program p2 = p;
  p2.push_back( Operation::Type::MOV, Operand::Type::DIRECT, 1, Operand::Type::DIRECT, 0 );
  for ( size_t i = 0; i < seqs.size(); i++ )
  {
    if ( settings.search_linear || !seqs[i].is_linear() )
    {
      if ( i == 1 )
      {
        findAll( p, seqs[i], result );
      }
      else
      {
        p2.ops.back().source.value = i;
        findAll( p2, seqs[i], result );
      }
    }
  }
  return result;
}

void Oeis::findAll( const Program &p, const Sequence &norm_seq, seq_programs_t &result ) const
{
  // collect possible matches
  seq_programs_t temp_result;
  Sequence full_seq;
  for ( size_t i = 0; i < matchers.size(); i++ )
  {
    temp_result.clear();
    matchers[i]->match( p, norm_seq, temp_result );

    // validate the found matches
    size_t j = 0;
    for ( auto t : temp_result )
    {
      matcher_stats[i].candidates++;
      auto &expected_full_seq = sequences.at( t.first ).full;
      try
      {
        full_seq.clear();
        interpreter.eval( t.second, full_seq, expected_full_seq.size() );
        if ( full_seq.size() != expected_full_seq.size() || full_seq != expected_full_seq )
        {
          matcher_stats[i].false_positives++;
          auto match_length = norm_seq.size();
          auto got = full_seq.subsequence( 0, match_length );
          auto exp = expected_full_seq.subsequence( 0, match_length );
          if ( got != exp )
          {
            auto id = sequences.at( t.first ).id_str();
            Log::get().error( matchers[i]->getName() + " matcher generates wrong program for " + id, false );
            Log::get().error( " -  expected: " + exp.to_string() );
            Log::get().error( " -       got: " + got.to_string() );
            Log::get().error( " - generated: " + norm_seq.to_string() );
            std::ofstream o1( "programs/debug/matcher/" + id + ".asm" );
            ProgramUtil::print( p, o1 );
            std::ofstream o2(
                "programs/debug/matcher/" + id + "-" + matchers[i]->getName() + "-" + std::to_string( j ) + ".asm" );
            ProgramUtil::print( t.second, o2 );
          }
        }
        else
        {
          // successful match!
          result.push_back( t );
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

void Oeis::dumpProgram( number_t id, Program p, const std::string &file ) const
{
  ProgramUtil::removeOps( p, Operation::Type::NOP );
  std::ofstream out( file );
  auto &seq = sequences.at( id );
  out << "; " << seq << std::endl;
  out << "; " << seq.full << std::endl;
  out << std::endl;
  ProgramUtil::print( p, out );
}

std::pair<bool, Program> Oeis::optimizeAndCheck( const Program &p, const OeisSequence &seq, bool optimize ) const
{
  // optimize and minimize program
  std::pair<bool, Program> optimized;
  optimized.first = true;
  optimized.second = p;
  if ( optimize )
  {
    optimizer.optimizeAndMinimize( optimized.second, 2, 1, seq.full.size() );
  }

  // check its correctness
  Sequence new_seq;
  try
  {
    interpreter.eval( optimized.second, new_seq, seq.full.size() );
    if ( seq.full.size() != new_seq.size() || seq.full != new_seq )
    {
      optimized.first = false;
    }
  }
  catch ( const std::exception &e )
  {
    optimized.first = false;
  }

  // throw error if not correct
  if ( !optimized.first )
  {
    std::string msg = "Program for " + seq.id_str() + " generates wrong result";
    if ( optimize )
    {
      msg = msg + " after optimization or minimization";
    }
    Log::get().error( msg, false );
    std::ofstream out( "programs/debug/optimizer/" + seq.id_str() + ".asm" );
    ProgramUtil::print( p, out );
  }

  return optimized;
}

std::pair<bool, bool> Oeis::updateProgram( number_t id, const Program &p ) const
{
  auto &seq = sequences.at( id );
  std::string file_name = getOeisFile( seq );
  bool is_new = true;
  std::pair<bool, Program> optimized;
  {
    std::ifstream in( file_name );
    if ( in.good() )
    {
      if ( settings.optimize_existing_programs )
      {
        optimized = optimizeAndCheck( p, seq, true );
        if ( !optimized.first )
        {
          return
          { false,false};
        }
        is_new = false;
        Parser parser;
        auto existing_program = parser.parse( in );
        if ( ProgramUtil::numOps( optimized.second, Operand::Type::INDIRECT ) > 0
            || ProgramUtil::numOps( optimized.second, false ) >= ProgramUtil::numOps( existing_program, false ) )
        {
          return
          { false,false};
        }
      }
      else
      {
        return
        { false,false};
      }
    }
  }
  if ( is_new )
  {
    optimized = optimizeAndCheck( p, seq, false );
    if ( !optimized.first )
    {
      return
      { false,false};
    }
  }
  std::stringstream buf;
  buf << "Found ";
  if ( is_new ) buf << "first";
  else buf << "shorter";
  buf << " program for " << seq << " First terms: " << static_cast<Sequence>( seq );
  Log::get().alert( buf.str() );
  dumpProgram( id, optimized.second, file_name );
  std::ofstream gen_args;
  gen_args.open( "programs/oeis/generator_args.txt", std::ios_base::app );
  gen_args << seq.id_str() << ": " << settings.getGeneratorArgs() << std::endl;
  return
  { true,is_new};
}

void Oeis::maintain( volatile sig_atomic_t &exit_flag )
{
  if ( !settings.optimize_existing_programs )
  {
    Log::get().error( "Option -x required to run maintenance", true );
  }
  Log::get().info( "Start maintaining OEIS programs" );
  const std::string readme( "README.md" );
  std::ifstream readme_in( readme );
  std::stringstream buffer;
  std::string str;
  while ( std::getline( readme_in, str ) )
  {
    buffer << str << std::endl;
    if ( str == "## Available Programs" )
    {
      break;
    }
  }
  readme_in.close();
  std::ofstream readme_out( readme );
  readme_out << buffer.str() << std::endl;
  std::ofstream list_file;
  int list_index = -1;
  size_t num_programs = 0;
  size_t num_optimized = 0;
  std::vector<size_t> num_programs_per_length;
  std::vector<size_t> num_ops_per_type( Operation::Types.size(), 0 );
  for ( auto &s : sequences )
  {
    std::string file_name = getOeisFile( s );
    std::ifstream file( file_name );
    if ( file.good() )
    {
      if ( exit_flag ) continue;
      Log::get().debug( "Checking program for " + s.to_string() );
      Parser parser;
      Program program = parser.parse( file );
      Settings settings2 = settings;
      settings2.num_terms = s.full.size();
      Interpreter interpreter( settings2 );
      bool okay;
      try
      {
        Sequence result;
        interpreter.eval( program, result );
        if ( result.size() != s.full.size() || result != s.full )
        {
          Log::get().error( "Program did not evaluate to expected sequence: " + file_name );
          okay = false;
        }
        else
        {
          okay = true;
        }
      }
      catch ( const std::exception &exc )
      {
        okay = false;
      }
      if ( !okay )
      {
        Log::get().warn( "Deleting program due to evaluation error: " + file_name );
        file.close();
        remove( file_name.c_str() );
      }
      else
      {
        ProgramUtil::removeOps( program, Operation::Type::NOP );
        Program optimized = program;
        Optimizer optimizer( settings2 );
        optimizer.optimizeAndMinimize( optimized, 2, 1, s.full.size() );
        if ( !(program == optimized) )
        {
          Log::get().warn( "Updating program because it is not optimal: " + file_name );
          num_optimized++;
        }
        dumpProgram( s.id, optimized, file_name );

        // collect stats
        num_programs++;
        const size_t num_ops = ProgramUtil::numOps( optimized, false );
        if ( num_ops >= num_programs_per_length.size() )
        {
          num_programs_per_length.resize( num_ops + 1 );
        }
        num_programs_per_length[num_ops]++;
        for ( auto &op : optimized.ops )
        {
          num_ops_per_type[static_cast<size_t>( op.type )]++;
        }

        // write list file
        if ( list_index < 0 || (int) s.id / 100000 != list_index )
        {
          list_index++;
          std::string list_path = "programs/oeis/list" + std::to_string( list_index ) + ".md";
          OeisSequence start( (list_index * 100000) + 1 );
          OeisSequence end( (list_index + 1) * 100000 );
          readme_out << "* [" << start.id_str() << "-" << end.id_str() << "](" << list_path << ")\n";
          list_file.close();
          list_file.open( list_path );
          list_file << "# Programs for " << start.id_str() << "-" << end.id_str() << "\n\n";
          list_file
              << "List of integer sequences with links to LODA programs. An _Ln_ program is a LODA program of length _n_."
              << "\n\n";
        }
        list_file << "* [" << s.id_str() << "](http://oeis.org/" << s.id_str() << ") ([L" << std::setw( 2 )
            << std::setfill( '0' ) << num_ops << " program](" << s.id_str() << ".asm)): " << s.name << "\n";
      }
    }
  }
  list_file.close();
  readme_out << "\n" << "Total number of programs: ";
  readme_out << num_programs << "/" << total_count_ << " (" << (int) (100 * num_programs / total_count_) << "%)\n\n";
  readme_out
      << "![LODA Program Length Distribution](https://raw.githubusercontent.com/ckrause/loda/master/stats/lengths.png)\n";
  // readme_out << "![LODA Program Counts](https://raw.githubusercontent.com/ckrause/loda/master/stats/counts.png)\n";
  readme_out.close();

  // write stats
  const std::string sep( "," );
  std::ofstream lengths( "stats/program_lengths.csv" );
  for ( size_t i = 0; i < num_programs_per_length.size(); i++ )
  {
    lengths << std::to_string( i ) << sep << std::to_string( num_programs_per_length[i] ) << "\n";
  }
  lengths.close();
  std::ofstream op_counts( "stats/operation_counts.csv" );
  for ( size_t i = 0; i < num_ops_per_type.size(); i++ )
  {
    op_counts << Operation::Metadata::get( static_cast<Operation::Type>( i ) ).name << sep
        << std::to_string( num_ops_per_type[i] ) << "\n";
  }
  op_counts.close();

  if ( num_optimized > 0 )
  {
    Log::get().alert( "Optimized " + std::to_string( num_optimized ) + " programs" );
  }
  Log::get().info( "Finished checking programs for OEIS sequences" );
}
