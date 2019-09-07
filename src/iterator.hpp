#include "program.hpp"

class Iterator
{
public:

  Iterator()
      :
      size_( 0 )
  {
  }

  Iterator( const Program &p )
      :
      p_( p ),
      size_( p.ops.size() )
  {
  }

  Program next();

private:

  bool inc( Operation &op );

  bool inc( Operand &o );

  Program p_;
  size_t size_;

};
