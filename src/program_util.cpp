#include "program_util.hpp"

#include "program.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>

void ProgramUtil::removeOps( Program &p, Operation::Type type )
{
  auto it = p.ops.begin();
  while ( it != p.ops.end() )
  {
    if ( it->type == type )
    {
      it = p.ops.erase( it );
    }
    else
    {
      it++;
    }
  }
}

bool ProgramUtil::replaceOps( Program &p, Operation::Type oldType, Operation::Type newType )
{
  bool result = false;
  for ( Operation &op : p.ops )
  {
    if ( op.type == oldType )
    {
      op.type = newType;
      result = true;
    }
  }
  return result;
}

bool ProgramUtil::isNop( const Operation& op )
{
  if ( op.type == Operation::Type::NOP || op.type == Operation::Type::DBG )
  {
    return true;
  }
  else if ( op.source == op.target
      && (op.type == Operation::Type::MOV || op.type == Operation::Type::MIN || op.type == Operation::Type::MAX) )
  {
    return true;
  }
  else if ( op.source.type == Operand::Type::CONSTANT && op.source.value == 0
      && (op.type == Operation::Type::ADD || op.type == Operation::Type::SUB) )
  {
    return true;
  }
  else if ( op.source.type == Operand::Type::CONSTANT && op.source.value == 1
      && ((op.type == Operation::Type::MUL || op.type == Operation::Type::DIV || op.type == Operation::Type::DIF
          || op.type == Operation::Type::POW || op.type == Operation::Type::BIN)) )
  {
    return true;
  }
  return false;
}

size_t ProgramUtil::numOps( const Program &p, bool withNops )
{
  if ( withNops )
  {
    return p.ops.size();
  }
  else
  {
    size_t num_ops = 0;
    for ( auto &op : p.ops )
    {
      if ( op.type != Operation::Type::NOP )
      {
        num_ops++;
      }
    }
    return num_ops;
  }
}

size_t ProgramUtil::numOps( const Program &p, Operation::Type type )
{
  size_t num_ops = 0;
  for ( auto &op : p.ops )
  {
    if ( op.type == type )
    {
      num_ops++;
    }
  }
  return num_ops;
}

size_t ProgramUtil::numOps( const Program &p, Operand::Type type )
{
  size_t num_ops = 0;
  for ( auto &op : p.ops )
  {
    auto &m = Operation::Metadata::get( op.type );
    if ( m.num_operands == 1 && op.target.type == type )
    {
      num_ops++;
    }
    else if ( m.num_operands == 2 && (op.source.type == type || op.target.type == type) )
    {
      num_ops++;
    }
  }
  return num_ops;
}

bool ProgramUtil::isArithmetic( Operation::Type t )
{
  // TODO: model this as metadata?
  return (t != Operation::Type::NOP && t != Operation::Type::DBG && t != Operation::Type::LPB
      && t != Operation::Type::LPE && t != Operation::Type::CLR && t != Operation::Type::CAL);
}

bool ProgramUtil::hasIndirectOperand( const Operation &op )
{
  const auto num_ops = Operation::Metadata::get( op.type ).num_operands;
  return (num_ops > 0 && op.target.type == Operand::Type::INDIRECT)
      || (num_ops > 1 && op.source.type == Operand::Type::INDIRECT);
}

bool ProgramUtil::areIndependent( const Operation& op1, const Operation& op2 )
{
  if ( !isArithmetic( op1.type ) && op1.type != Operation::Type::CAL )
  {
    return false;
  }
  if ( !isArithmetic( op2.type ) && op2.type != Operation::Type::CAL )
  {
    return false;
  }
  if ( hasIndirectOperand( op1 ) || hasIndirectOperand( op2 ) )
  {
    return false;
  }
  if ( op1.target.value == op2.target.value )
  {
    return false;
  }
  if ( op1.source.type == Operand::Type::DIRECT && op2.target.value == op1.source.value )
  {
    return false;
  }
  if ( op2.source.type == Operand::Type::DIRECT && op1.target.value == op2.source.value )
  {
    return false;
  }
  return true;
}

bool ProgramUtil::getUsedMemoryCells( const Program &p, std::unordered_set<int64_t> &used_cells, int64_t &largest_used,
    size_t max_memory )
{
  for ( auto &op : p.ops )
  {
    int64_t region_length = 1;
    if ( op.source.type == Operand::Type::INDIRECT || op.target.type == Operand::Type::INDIRECT )
    {
      return false;
    }
    if ( op.type == Operation::Type::LPB || op.type == Operation::Type::CLR )
    {
      if ( op.source.type == Operand::Type::CONSTANT )
      {
        region_length = op.source.value.asInt();
      }
      else
      {
        return false;
      }
    }
    if ( region_length > static_cast<int64_t>( max_memory ) )
    {
      return false;
    }
    if ( op.source.type == Operand::Type::DIRECT )
    {
      for ( int64_t i = 0; i < region_length; i++ )
      {
        used_cells.insert( op.source.value.asInt() + i );
      }
    }
    if ( op.target.type == Operand::Type::DIRECT )
    {
      for ( int64_t i = 0; i < region_length; i++ )
      {
        used_cells.insert( op.target.value.asInt() + i );
      }
    }
  }
  largest_used = used_cells.empty() ? 0 : *used_cells.begin();
  for ( number_t used : used_cells )
  {
    largest_used = std::max( largest_used, used );
  }
  return true;
}

std::string getIndent( int indent )
{
  std::string s;
  for ( int i = 0; i < indent; i++ )
  {
    s += " ";
  }
  return s;
}

std::string ProgramUtil::operandToString( const Operand &op )
{
  switch ( op.type )
  {
  case Operand::Type::CONSTANT:
    return op.value.to_string();
  case Operand::Type::DIRECT:
    return "$" + op.value.to_string();
  case Operand::Type::INDIRECT:
    return "$$" + op.value.to_string();
  }
  return "";
}

std::string ProgramUtil::operationToString( const Operation &op )
{
  auto &metadata = Operation::Metadata::get( op.type );
  std::string str;
  if ( metadata.num_operands == 0 && op.type != Operation::Type::NOP )
  {
    str = metadata.name;
  }
  else if ( metadata.num_operands == 1
      || (op.type == Operation::Type::LPB && op.source.type == Operand::Type::CONSTANT && op.source.value == 1) ) // lpb has an optional second argument
  {
    str = metadata.name + " " + operandToString( op.target );
  }
  else if ( metadata.num_operands == 2 )
  {
    str = metadata.name + " " + operandToString( op.target ) + "," + operandToString( op.source );
  }
  if ( !op.comment.empty() )
  {
    if ( !str.empty() )
    {
      str = str + " ";
    }
    str = str + "; " + op.comment;
  }
  return str;
}

void ProgramUtil::print( const Operation &op, std::ostream &out, int indent )
{
  out << getIndent( indent ) << operationToString( op );
}

void ProgramUtil::print( const Program &p, std::ostream &out, std::string newline )
{
  int indent = 0;
  for ( auto &op : p.ops )
  {
    if ( op.type == Operation::Type::LPE )
    {
      indent -= 2;
    }
    print( op, out, indent );
    out << newline;
    if ( op.type == Operation::Type::LPB )
    {
      indent += 2;
    }
  }
}

void ProgramUtil::exportToDot( const Program& p, std::ostream &out )
{
  out << "digraph G {" << std::endl;

  // merge operations
  std::vector<std::vector<Operation>> merged;
  merged.push_back( { } );
  for ( auto op : p.ops )
  {
    if ( op.type == Operation::Type::NOP )
    {
      continue;
    }
    if ( !merged.back().empty() && !areIndependent( op, merged.back().back() ) )
    {
      merged.push_back( { } );
    }
    op.comment.clear();
    merged.back().push_back( op );
  }

  // insert forks and joins
  for ( size_t i = 0; i < merged.size(); i++ )
  {
    if ( merged[i].size() > 1 )
    {
      merged.insert( merged.begin() + i, { Operation( Operation::Type::NOP, { }, { }, "triangle" ) } );
      merged.insert( merged.begin() + i + 2, { Operation( Operation::Type::NOP, { }, { }, "invtriangle" ) } );
      i += 2;
    }
  }

  // nodes
  for ( size_t i = 0; i < merged.size(); i++ )
  {
    for ( size_t j = 0; j < merged[i].size(); j++ )
    {
      std::string shape = "ellipse";
      std::string color = "black";
      std::string fontname = "courier";
      std::string label = operationToString( merged[i][j] );
      if ( merged[i][j].type == Operation::Type::NOP )
      {
        shape = merged[i][j].comment;
        label.clear();
      }
      else if ( merged[i][j].type == Operation::Type::MOV )
      {
        color = "blue";
      }
      else if ( isArithmetic( merged[i][j].type ) )
      {
        color = "green";
      }
      else
      {
        color = "red";
      }
      out << "  o" << i << "_" << j << " [label=\"" << label
          << "\",shape=" + shape + ",color=" + color + ",fontname=\"" + fontname + "\"];" << std::endl;
    }
  }

  // edges
  std::stack<std::string> lpbs;
  std::vector<std::string> targets;

  for ( size_t i = 0; i < merged.size(); i++ )
  {
    for ( size_t j = 0; j < merged[i].size(); j++ )
    {
      // current source node
      std::string src = "o" + std::to_string( i ) + "_" + std::to_string( j );
      // target nodes
      targets.clear();
      if ( i + 1 < merged.size() ) // edge to next node
      {
        for ( size_t k = 0; k < merged[i + 1].size(); k++ )
        {
          targets.push_back( "o" + std::to_string( i + 1 ) + "_" + std::to_string( k ) );
        }
      }
      if ( merged[i][j].type == Operation::Type::LPE ) // edge back to loop start
      {
        targets.push_back( lpbs.top() );
        lpbs.pop();
      }
      if ( !targets.empty() )
      {
        out << "  " << src << " -> {";
        for ( auto t : targets )
        {
          out << " " << t;
        }
        out << " }" << std::endl;
      }
      if ( merged[i][j].type == Operation::Type::LPB )
      {
        lpbs.push( "o" + std::to_string( i ) + "_" + std::to_string( j ) );
      }
    }

  }
  out << "}" << std::endl;
}

size_t ProgramUtil::hash( const Program &p )
{
  size_t h = 0;
  for ( auto &op : p.ops )
  {
    if ( op.type != Operation::Type::NOP )
    {
      h = (h * 3) + hash( op );
    }
  }
  return h;
}

size_t ProgramUtil::hash( const Operation &op )
{
  auto &meta = Operation::Metadata::get( op.type );
  size_t h = static_cast<size_t>( op.type );
  if ( meta.num_operands > 0 )
  {
    h = (5 * h) + hash( op.target );
  }
  if ( meta.num_operands > 1 )
  {
    h = (7 * h) + hash( op.source );
  }
  return h;
}

size_t ProgramUtil::hash( const Operand &op )
{
  return (11 * static_cast<size_t>( op.type )) + op.value.hash();
}

void throwInvalidLoop( const Program& p )
{
  throw std::runtime_error( "invalid loop" );
}

void ProgramUtil::validate( const Program& p )
{
  // validate number of open/closing loops
  int64_t open_loops = 0;
  auto it = p.ops.begin();
  while ( it != p.ops.end() )
  {
    if ( it->type == Operation::Type::LPB )
    {
      open_loops++;
    }
    else if ( it->type == Operation::Type::LPE )
    {
      if ( open_loops == 0 )
      {
        throwInvalidLoop( p );
      }
      open_loops--;
    }
    it++;
  }
  if ( open_loops != 0 )
  {
    throwInvalidLoop( p );
  }
}
