#include "interpreter.hpp"
#include "printer.hpp"

int main( void )
{
  Program p;

  // n:0, one:1, u:2, v:3, w:4

  // set one,1
  // cpy u,n
  // cpy v,n
  // sub u,one
  // sub v,u
  // set w,1
  // lpb v
  //   sub v,one
  //   add w,one
  // lpe

  p.var_names[0] = "n";
  p.var_names[1] = "one";
  p.var_names[2] = "u";
  p.var_names[3] = "v";
  p.var_names[4] = "w";

  p.ops.resize( 10 );
  p.ops[0].reset( new Mov( 1, Argument::Constant( 1 ) ) );
  p.ops[1].reset( new Mov( 2, Argument::Variable( 0 ) ) );
  p.ops[2].reset( new Mov( 3, Argument::Variable( 0 ) ) );
  p.ops[3].reset( new Sub( 2, Argument::Variable( 1 ) ) );
  p.ops[4].reset( new Sub( 3, Argument::Variable( 2 ) ) );
  p.ops[5].reset( new Mov( 4, Argument::Constant( 1 ) ) );
  p.ops[6].reset( new LoopBegin( { 3 } ) );
  p.ops[7].reset( new Sub( 3, Argument::Variable( 1 ) ) );
  p.ops[8].reset( new Sub( 4, Argument::Variable( 1 ) ) );
  p.ops[9].reset( new LoopEnd() );

  /*  p.ops[0].reset( new Set( 42, 0 ) );
   p.ops[1].reset( new Set( 3, 1 ) );
   p.ops[2].reset( new Set( 1, 2 ) );
   p.ops[3].reset( new LoopBegin( { 1 } ) );
   p.ops[4].reset( new Add( 0, 3 ) );
   p.ops[5].reset( new Sub( 2, 1 ) );
   p.ops[6].reset( new LoopEnd() );
   p.ops[7].reset( new Add( 2, 3 ) );
   */

  try
  {

    Printer r;
    r.Print( p, std::cout );
    std::cout << std::endl;

    Memory m;
    m.regs[0] = 8;

    Interpreter i;
    i.Run( p, m );

    std::cout << "out: " << m << std::endl;
  }
  catch ( const std::exception& e )
  {
    std::cerr << "error: " << std::string( e.what() ) << std::endl;
    return 1;
  }

  return EXIT_SUCCESS;
}

