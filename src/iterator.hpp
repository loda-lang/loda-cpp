#include "program.hpp"

class Iterator
{
  Iterator()
  {
  }

  Iterator( Program& p )
      : p_( p )
  {
  }

  Program next();

private:
  Program p_;

};
