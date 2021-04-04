#include "program.hpp"

class Iterator
{
public:

  Iterator()
      : size_( 0 )
  {
  }

  Iterator( const Program &p )
      : p_( p ),
        size_( p.ops.size() )
  {
  }

  Program next();

private:

  static const Operand CONSTANT_ZERO;
  static const Operand CONSTANT_ONE;
  static const Operand DIRECT_ZERO;
  static const Operand DIRECT_ONE;

  static const Operand SMALLEST_SOURCE;
  static const Operand SMALLEST_TARGET;
  static const Operation SMALLEST_OPERATION;

  bool inc( Operand &o );

  bool inc( Operation &op );

  bool incWithSkip( Operation &op );

  static bool shouldSkip( const Operation& op );

  Program p_;
  size_t size_;

};
