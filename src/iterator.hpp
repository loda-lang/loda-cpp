#include "program.hpp"

class Iterator
{
public:

  Iterator()
  {
  }

  Iterator( const Program& p )
      : p_( p )
  {
  }

  Program next();

private:
  Program p_;

};
