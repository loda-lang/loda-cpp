#include "blocks.hpp"

#include "parser.hpp"
#include "program_util.hpp"

#include <fstream>

Blocks::Interface::Interface()
{
}

Blocks::Interface::Interface( const Program& p )
{
  for ( auto& op : p.ops )
  {
    extend( op );
  }
}

void Blocks::Interface::extend( const Operation& op )
{
  auto& meta = Operation::Metadata::get( op.type );
  if ( meta.num_operands > 0 && op.target.type == Operand::Type::DIRECT )
  {
    if ( meta.is_reading_target )
    {
      inputs.insert( op.target.value );
      all.insert( op.target.value );
    }
    if ( meta.is_writing_target )
    {
      outputs.insert( op.target.value );
      all.insert( op.target.value );
    }
  }
  if ( meta.num_operands > 1 && op.source.type == Operand::Type::DIRECT )
  {
    inputs.insert( op.source.value );
    all.insert( op.source.value );
  }
}

void Blocks::Interface::clear()
{
  inputs.clear();
  outputs.clear();
  all.clear();
}

void Blocks::Collector::add( const Program& p )
{
  interface.clear();
  Program block;
  for ( auto op : p.ops )
  {
    if ( op.type == Operation::Type::NOP )
    {
      continue;
    }
    op.comment.clear();

    // decide whether and where to cut
    bool include_now = true;
    bool next_block = false;
    if ( op.type == Operation::Type::LPB )
    {
      include_now = false;
      next_block = true;
    }
    if ( op.type == Operation::Type::LPE )
    {
      next_block = true;
    }
    interface.extend( op );
    if ( interface.all.size() > 3 ) // magic number
    {
      include_now = false;
      next_block = true;
    }

    // append to block and cut if needed
    if ( include_now )
    {
      block.ops.push_back( op );
    }
    if ( next_block )
    {
      if ( !block.ops.empty() )
      {
        if ( block.ops.front().type == Operation::Type::LPB && block.ops.back().type != Operation::Type::LPE )
        {
          block.ops.erase( block.ops.begin(), block.ops.begin() + 1 );
        }
        if ( block.ops.back().type == Operation::Type::LPE && block.ops.front().type != Operation::Type::LPB )
        {
          block.ops.pop_back();
        }
        if ( !block.ops.empty() )
        {
          blocks[block]++;
          block.ops.clear();
        }
      }
      interface.clear();
    }
    if ( !include_now )
    {
      block.ops.push_back( op );
    }
  }

  // final block
  if ( !block.ops.empty() )
  {
    blocks[block]++;
  }
}

Blocks Blocks::Collector::finalize()
{
  Blocks result;
  for ( auto it : blocks )
  {
    Operation nop( Operation::Type::NOP );
    nop.comment = std::to_string( it.second );
    result.list.ops.push_back( nop );
    result.list.ops.insert( result.list.ops.end(), it.first.ops.begin(), it.first.ops.end() );
  }
  blocks.clear();
  result.initRatesAndOffsets();
  return result;
}

bool Blocks::Collector::empty() const
{
  return blocks.empty();
}

void Blocks::load( const std::string &path )
{
  Parser parser;
  list = parser.parse( path );
  initRatesAndOffsets();
}

void Blocks::save( const std::string &path )
{
  std::ofstream out( path );
  ProgramUtil::print( list, out );
}

Program Blocks::getBlock( size_t index ) const
{
  Program block;
  size_t offset = offsets.at( index ) + 1; // skip rate comment
  while ( offset < list.ops.size() && list.ops[offset].type != Operation::Type::NOP )
  {
    block.ops.push_back( list.ops[offset++] );
  }
  return block;
}

void Blocks::initRatesAndOffsets()
{
  offsets.clear();
  rates.clear();
  for ( size_t i = 0; i < list.ops.size(); i++ )
  {
    if ( list.ops[i].type == Operation::Type::NOP && !list.ops[i].comment.empty() )
    {
      offsets.push_back( i );
      rates.push_back( std::stoll( list.ops[i].comment ) );
    }
  }
}
