#include "serializer.hpp"

#include "common.hpp"

inline uint16_t operationTypeToInt( Operation::Type t )
{
  switch ( t )
  {
  case Operation::Type::NOP:
    return 0;
  case Operation::Type::MOV:
    return 1;
  case Operation::Type::ADD:
    return 2;
  case Operation::Type::SUB:
    return 3;
  case Operation::Type::LPB:
    return 4;
  case Operation::Type::LPE:
    return 5;
  case Operation::Type::CLR:
    return 6;
  case Operation::Type::DBG:
    return 7;
  }
  return 0; // unreachable
}

inline Operation::Type intToOperationType( uint16_t w )
{
  switch ( w )
  {
  case 0:
    return Operation::Type::NOP;
  case 1:
    return Operation::Type::MOV;
  case 2:
    return Operation::Type::ADD;
  case 3:
    return Operation::Type::SUB;
  case 4:
    return Operation::Type::LPB;
  case 5:
    return Operation::Type::LPE;
  case 6:
    return Operation::Type::CLR;
  case 7:
    return Operation::Type::DBG;
  default:
    break;
  }
  return Operation::Type::NOP; // unreachable
}

inline uint16_t operandTypesToInt( Operand::Type t, Operand::Type s )
{
  switch ( t )
  {
  case Operand::Type::CONSTANT:
    throw std::runtime_error( "constants not allowed as target operands" );
  case Operand::Type::MEM_ACCESS_DIRECT:
    switch ( s )
    {
    case Operand::Type::CONSTANT:
      return 0;
    case Operand::Type::MEM_ACCESS_DIRECT:
      return 1;
    case Operand::Type::MEM_ACCESS_INDIRECT:
      return 2;
    }
    break;
  case Operand::Type::MEM_ACCESS_INDIRECT:
    switch ( s )
    {
    case Operand::Type::CONSTANT:
      return 3;
    case Operand::Type::MEM_ACCESS_DIRECT:
      return 4;
    case Operand::Type::MEM_ACCESS_INDIRECT:
      return 5;
    }
    break;
  }
  return 0;
}

#define make_optype_pair(t,s) std::make_pair<Operand::Type, Operand::Type>( Operand::Type::t, Operand::Type::s )

inline std::pair<Operand::Type, Operand::Type> intToOperandTypes( uint16_t w )
{
  switch ( w )
  {
  case 0:
    return make_optype_pair( MEM_ACCESS_DIRECT, CONSTANT );
  case 1:
    return make_optype_pair( MEM_ACCESS_DIRECT, MEM_ACCESS_DIRECT );
  case 2:
    return make_optype_pair( MEM_ACCESS_DIRECT, MEM_ACCESS_INDIRECT );
  case 3:
    return make_optype_pair( MEM_ACCESS_INDIRECT, CONSTANT );
  case 4:
    return make_optype_pair( MEM_ACCESS_INDIRECT, MEM_ACCESS_DIRECT );
  case 5:
    return make_optype_pair( MEM_ACCESS_INDIRECT, MEM_ACCESS_INDIRECT );
  }
  return
  {}; // unreachable
}

uint16_t writeOperation( const Operation& op )
{
  uint16_t w = operationTypeToInt( op.type ) << 13;
  w |= operandTypesToInt( op.target.type, op.source.type ) << 10;
  if ( op.target.value > 31 || op.source.value > 31 )
  {
    throw std::runtime_error( "operand value exceeds limit of 31" );
  }
  w |= op.target.value << 5;
  w |= op.source.value;
  return w;
}

Operation readOperation( uint16_t w )
{
  Operation op( intToOperationType( w >> 13 ) );
  auto optypes = intToOperandTypes( (w >> 10) & 7 );
  op.target = Operand( optypes.first, (w >> 5) & 31 );
  op.source = Operand( optypes.second, w & 31 );
  return op;
}

void Serializer::writeProgram( const Program& p, std::ostream& out )
{
  std::vector<uint16_t> data;
  data.reserve( p.ops.size() );
  if ( p.ops.empty() || p.ops.front().type != Operation::Type::CLR )
  {
    data.push_back( writeOperation( Operation( Operation::Type::CLR ) ) );
  }
  for ( auto& op : p.ops )
  {
    data.push_back( writeOperation( op ) );
  }
  out.write( reinterpret_cast<const char*>( data.data() ), sizeof(uint16_t) * data.size() );
}

inline void findStart( std::istream& in )
{
  static uint16_t start = writeOperation( Operation( Operation::Type::CLR ) );
  uint16_t w;
  while ( true )
  {
    in.read( reinterpret_cast<char*>( &w ), sizeof(uint16_t) );
    if ( in.fail() )
    {
      throw std::runtime_error( "read error" );
    }
    if ( in.eof() )
    {
      throw std::runtime_error( "end of file" );
    }
    else if ( w == start )
    {
      return;
    }
    int new_pos = static_cast<int>( in.tellg() ) - 4;
    if ( new_pos < 0 )
    {
      throw std::runtime_error( "no program found" );
    }
    in.seekg( new_pos );
  }
}

void Serializer::readProgram( Program& p, std::istream& in )
{
  findStart( in );
  p.ops.clear();
  uint16_t w;
  while ( true )
  {
    in.read( reinterpret_cast<char*>( &w ), sizeof(uint16_t) );
    if ( in.eof() )
    {
      return;
    }
    if ( in.fail() )
    {
      throw std::runtime_error( "read error" );
    }
    Operation op = readOperation( w );
    if ( op.type == Operation::Type::CLR )
    {
      return;
    }
    p.ops.push_back( op );
  }
}
