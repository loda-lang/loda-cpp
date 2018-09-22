#pragma once

#include "program.hpp"

class Serializer
{

  uint16_t WriteOperation( const Operation& op );

  Operation ReadOperation( uint16_t w );

};
