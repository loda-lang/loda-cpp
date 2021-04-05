#include "program.hpp"

class Iterator
{
public:

  Iterator()
      : size( 0 ),
        skipped( 0 )
  {
  }

  Iterator( const Program &p )
      : program( p ),
        size( p.ops.size() ),
        skipped( 0 )
  {
  }

  Program next();

  size_t getSkipped() const
  {
    return skipped;
  }

private:

  static const Operand CONSTANT_ZERO;
  static const Operand CONSTANT_ONE;
  static const Operand DIRECT_ZERO;
  static const Operand DIRECT_ONE;

  static const Operand SMALLEST_SOURCE;
  static const Operand SMALLEST_TARGET;
  static const Operation SMALLEST_OPERATION;

  bool inc( Operand &o, bool direct );

  bool inc( Operation &op );

  bool incWithSkip( Operation &op );

  void doNext();

  static bool shouldSkip( const Operation& op );

  Program program;
  size_t size;
  size_t skipped;

};
