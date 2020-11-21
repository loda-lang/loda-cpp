#include "extender.hpp"

#include "optimizer.hpp"

void add_or_sub( Program &p, number_t c )
{
  if ( c > 0 )
  {
    p.push_back( Operation::Type::ADD, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::CONSTANT, c );
  }
  else if ( c < 0 )
  {
    p.push_back( Operation::Type::SUB, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::CONSTANT, -c );
  }
}

bool Extender::linear1( Program &p, line_t inverse, line_t target )
{
  if ( inverse.offset == target.offset && inverse.factor == target.factor )
  {
    return true;
  }
  if ( inverse.offset != 0 )
  {
    add_or_sub( p, -inverse.offset );
  }
  if ( inverse.factor != 1 )
  {
    p.push_back( Operation::Type::DIV, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::CONSTANT,
        inverse.factor );
  }
  if ( target.factor != 1 )
  {
    p.push_back( Operation::Type::MUL, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::CONSTANT,
        target.factor );
  }
  if ( target.offset != 0 )
  {
    add_or_sub( p, target.offset );
  }
  return true;
}

bool Extender::linear2( Program &p, line_t inverse, line_t target )
{
  if ( inverse.factor == target.factor && inverse.offset == target.offset )
  {
    return true;
  }
  if ( inverse.factor != 1 )
  {
    p.push_back( Operation::Type::DIV, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::CONSTANT,
        inverse.factor );
  }
  add_or_sub( p, target.offset - inverse.offset );
  if ( target.factor != 1 )
  {
    p.push_back( Operation::Type::MUL, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::CONSTANT,
        target.factor );
  }
  return true;
}

bool Extender::polynomial( Program &p, const Polynomial &diff )
{
  Settings settings;
  Optimizer optimizer( settings );

  // constant term
  if ( diff.size() > 0 )
  {
    add_or_sub( p, diff[0] );
  }

  // non-constant terms
  if ( diff.size() > 1 )
  {
    std::unordered_set<number_t> used_cells;
    number_t max_cell = 0;
    if ( !optimizer.getUsedMemoryCells( p, used_cells, max_cell ) )
    {
      return false;
    }
    max_cell = std::max( max_cell, (number_t) Program::OUTPUT_CELL );
    const number_t saved_arg_cell = max_cell + 1;
    const number_t x_cell = max_cell + 2;
    const number_t term_cell = max_cell + 3;

    // save argument
    p.push_front( Operation::Type::MOV, Operand::Type::DIRECT, saved_arg_cell, Operand::Type::DIRECT, 0 );

    // polynomial evaluation code
    for ( size_t exp = 1; exp < diff.size(); exp++ )
    {
      // update x^exp
      if ( exp == 1 )
      {
        p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, x_cell, Operand::Type::DIRECT, saved_arg_cell );
      }
      else
      {
        p.push_back( Operation::Type::MUL, Operand::Type::DIRECT, x_cell, Operand::Type::DIRECT, saved_arg_cell );
      }

      // update result
      const auto factor = diff[exp];
      if ( factor > 0 )
      {
        p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, term_cell, Operand::Type::DIRECT, x_cell );
        p.push_back( Operation::Type::MUL, Operand::Type::DIRECT, term_cell, Operand::Type::CONSTANT, factor );
        p.push_back( Operation::Type::ADD, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::DIRECT,
            term_cell );
      }
      else if ( factor < 0 )
      {
        return false;
      }
    }
  }
  return true;
}

bool Extender::delta_one( Program &p, const bool sum )
{
  Settings settings;
  Optimizer optimizer( settings );
  std::unordered_set<number_t> used_cells;
  number_t largest_used = 0;
  if ( !optimizer.getUsedMemoryCells( p, used_cells, largest_used ) )
  {
    return false;
  }
  largest_used = std::max( (number_t) Program::OUTPUT_CELL, largest_used );
  auto saved_arg_cell = largest_used + 1;
  auto saved_result_cell = largest_used + 2;
  auto loop_counter_cell = largest_used + 3;
  auto tmp_counter_cell = largest_used + 4;

  Program prefix;
  prefix.push_back( Operation::Type::MOV, Operand::Type::DIRECT, saved_arg_cell, Operand::Type::DIRECT, 0 );
  if ( sum )
  {
    prefix.push_back( Operation::Type::MOV, Operand::Type::DIRECT, loop_counter_cell, Operand::Type::DIRECT, 0 );
    prefix.push_back( Operation::Type::ADD, Operand::Type::DIRECT, loop_counter_cell, Operand::Type::CONSTANT, 1 );
  }
  else
  {
    prefix.push_back( Operation::Type::MOV, Operand::Type::DIRECT, loop_counter_cell, Operand::Type::CONSTANT, 2 );
  }
  prefix.push_back( Operation::Type::LPB, Operand::Type::DIRECT, loop_counter_cell, Operand::Type::CONSTANT, 1 );
  prefix.push_back( Operation::Type::CLR, Operand::Type::DIRECT, 0, Operand::Type::CONSTANT, largest_used + 1 );
  prefix.push_back( Operation::Type::SUB, Operand::Type::DIRECT, loop_counter_cell, Operand::Type::CONSTANT, 1 );
  prefix.push_back( Operation::Type::MOV, Operand::Type::DIRECT, 0, Operand::Type::DIRECT, saved_arg_cell );
  if ( sum )
  {
    prefix.push_back( Operation::Type::SUB, Operand::Type::DIRECT, 0, Operand::Type::DIRECT, loop_counter_cell );
  }
  else
  {
    prefix.push_back( Operation::Type::ADD, Operand::Type::DIRECT, 0, Operand::Type::DIRECT, loop_counter_cell );
    prefix.push_back( Operation::Type::SUB, Operand::Type::DIRECT, 0, Operand::Type::CONSTANT, 1 );
  }
  p.ops.insert( p.ops.begin(), prefix.ops.begin(), prefix.ops.end() );

  if ( sum )
  {
    p.push_back( Operation::Type::ADD, Operand::Type::DIRECT, saved_result_cell, Operand::Type::DIRECT,
        Program::OUTPUT_CELL );
  }
  else
  {
    p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, tmp_counter_cell, Operand::Type::DIRECT,
        loop_counter_cell );
    p.push_back( Operation::Type::LPB, Operand::Type::DIRECT, tmp_counter_cell, Operand::Type::CONSTANT, 1 );
    p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, saved_result_cell, Operand::Type::DIRECT,
        Program::OUTPUT_CELL );
    p.push_back( Operation::Type::SUB, Operand::Type::DIRECT, tmp_counter_cell, Operand::Type::CONSTANT, 1 );
    p.push_back( Operation::Type::LPE, Operand::Type::CONSTANT, 0, Operand::Type::CONSTANT, 0 );
  }
  p.push_back( Operation::Type::LPE, Operand::Type::CONSTANT, 0, Operand::Type::CONSTANT, 0 );

  if ( sum )
  {
    p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::DIRECT,
        saved_result_cell );
  }
  else
  {
    p.push_back( Operation::Type::LPB, Operand::Type::DIRECT, saved_arg_cell, Operand::Type::CONSTANT, 1 );
    p.push_back( Operation::Type::SUB, Operand::Type::DIRECT, saved_result_cell, Operand::Type::DIRECT,
        Program::OUTPUT_CELL );
    p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, saved_arg_cell, Operand::Type::CONSTANT, 0 );
    p.push_back( Operation::Type::LPE, Operand::Type::CONSTANT, 0, Operand::Type::CONSTANT, 0 );
    p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::DIRECT,
        saved_result_cell );
  }
  return true;
}

bool Extender::delta_it( Program &p, int delta )
{
  while ( delta < 0 )
  {
    if ( !delta_one( p, false ) )
    {
      return false;
    }
    delta++;
  }
  while ( delta > 0 )
  {
    if ( !delta_one( p, true ) )
    {
      return false;
    }
    delta--;
  }
  return true;
}
