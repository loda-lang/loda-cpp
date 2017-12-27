#include "interpreter.hpp"
#include "parser.hpp"
#include "printer.hpp"

int main( void )
{
  Program p;

  // n:0, u:2, v:3, w:4

  // mov u,n
  // mov v,n
  // sub u,1
  // sub v,u
  // set w,1
  // lpb v
  //   sub v,1
  //   sub w,1
  // lpe

  p.var_names[0] = "n";
  p.var_names[2] = "u";
  p.var_names[3] = "v";
  p.var_names[4] = "w";

  p.ops.resize( 9 );
  p.ops[0].reset( new Mov( 2, Argument::Variable( 0 ) ) );
  p.ops[1].reset( new Mov( 3, Argument::Variable( 0 ) ) );
  p.ops[2].reset( new Sub( 2, Argument::Constant( 1 ) ) );
  p.ops[3].reset( new Sub( 3, Argument::Variable( 2 ) ) );
  p.ops[4].reset( new Mov( 4, Argument::Constant( 1 ) ) );
  p.ops[5].reset( new LoopBegin( { 3 } ) );
  p.ops[6].reset( new Sub( 3, Argument::Constant( 1 ) ) );
  p.ops[7].reset( new Sub( 4, Argument::Constant( 1 ) ) );
  p.ops[8].reset( new LoopEnd() );

  Printer r;

  try
  {

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

  Parser pa;
  auto q = pa.Parse( "test/test1.loda" );

  std::cout << "PARSED: " << std::endl;
  r.Print( *q, std::cout );

  return EXIT_SUCCESS;
}

