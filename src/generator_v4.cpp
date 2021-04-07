#include "generator_v4.hpp"
#include "generator.hpp"

#include "parser.hpp"
#include "program_util.hpp"
#include "util.hpp"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>

std::string getGenV4Home()
{
  // no trailing / here
  return getLodaHome() + "gen_v4";
}

std::string getNumFilesPath()
{
  return getGenV4Home() + "/numfiles.txt";
}

ProgramState::ProgramState( int64_t index )
    : index( index ),
      generated( 0 )
{
  if ( index < 1 || index >= 10000 )
  {
    throw std::runtime_error( "invalid program state index: " + std::to_string( index ) );
  }
}

std::string ProgramState::getPath() const
{
  std::stringstream s;
  s << getGenV4Home() << "/S" << std::setw( 4 ) << std::setfill( '0' ) << index << ".txt";
  return s.str();
}

void ProgramState::load()
{
  Parser parser;
  Program p = parser.parse( getPath() );
  size_t step = 0;
  start.ops.clear();
  current.ops.clear();
  end.ops.clear();
  for ( auto& op : p.ops )
  {
    if ( op.type == Operation::Type::NOP && !op.comment.empty() )
    {
      if ( op.comment == "start" )
      {
        step = 1;
      }
      else if ( op.comment.rfind( "current: ", 0 ) == 0 )
      {
        step = 2;
        auto sub = op.comment.substr( 9 );
        std::cout << sub << std::endl;
        generated = stoll( sub );
      }
      else if ( op.comment == "end" )
      {
        step = 3;
      }
      else
      {
        std::runtime_error( "program state load error" );
      }
      continue;
    }
    switch ( step )
    {
    case 1:
      start.ops.push_back( op );
      break;
    case 2:
      current.ops.push_back( op );
      break;
    case 3:
      end.ops.push_back( op );
      break;
    default:
      std::runtime_error( "program state load error" );
    }
  }
}

void ProgramState::save() const
{
  Program p;
  Operation nop( Operation::Type::NOP );
  nop.comment = "start";
  p.ops.push_back( nop );
  p.ops.insert( p.ops.end(), start.ops.begin(), start.ops.end() );
  nop.comment = "current: " + std::to_string( generated );
  p.ops.push_back( nop );
  p.ops.insert( p.ops.end(), current.ops.begin(), current.ops.end() );
  nop.comment = "end";
  p.ops.push_back( nop );
  p.ops.insert( p.ops.end(), end.ops.begin(), end.ops.end() );
  std::ofstream f( getPath() );
  ProgramUtil::print( p, f );
  f.close();
}

GeneratorV4::GeneratorV4( const Config &config, const Stats &stats, int64_t seed )
    : Generator( config, stats, seed )
{
  init();
}

Program GeneratorV4::generateProgram()
{
  Program p = iterator.next();
  return p;
}

void GeneratorV4::init()
{
  // obtain lock
//  FolderLock lock( getGenV4Home() );

//  static const size_t num_files = 200;
//  std::ofstream nf_out( getNumFilesPath() );
//  nf_out << num_files << std::endl;
//  nf_out.close();
}

std::pair<Operation, double> GeneratorV4::generateOperation()
{
  throw std::runtime_error( "unsupported operation in generator v4" );
  return
  {};
}
