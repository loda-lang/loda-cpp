#include "serializer.hpp"

uint16_t OperationTypeToInt( Operation::Type t )
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
  case Operation::Type::DBG:
    return 6;
  case Operation::Type::END:
    return 7;
  }
  return 0; // unreachable
}

Operation::Type IntToOperationType( uint16_t w )
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
    return Operation::Type::DBG;
  case 7:
    return Operation::Type::END;
  default:
    break;
  }
  return Operation::Type::NOP; // unreachable
}

uint16_t Serializer::WriteOperation( const Operation& op )
{
  uint16_t w = OperationTypeToInt( op.type ) << 13;

  return w;
}

Operation Serializer::ReadOperation( uint16_t w )
{
  Operation op( IntToOperationType( w >> 3 ) );

  return op;

}
