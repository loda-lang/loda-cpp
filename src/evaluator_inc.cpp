#include "evaluator_inc.hpp"

#include "program_util.hpp"
#include "semantics.hpp"

/*
; A079309: a(n) = C(1,1) + C(3,2) + C(5,3) + ... + C(2*n-1,n).
;
1,4,14,49,175,637,2353,8788,33098,125476,478192,1830270,7030570,27088870,104647630,405187825,1571990935,6109558585,23782190485,92705454895,361834392115,1413883873975,5530599237775,21654401079325,84859704298201,332818970772253,1306288683596309,5130633983976529,20164267233747049,79296558016177761,312010734643808305,1228322805115103572,4838037022123236442,19064557759743524812,75157696668074947528,296413966806493337130,1169479248974306442046,4615789573320937119346,18224297007920453127146

; PRE_LOOP
add $0,1
mov $1,2

lpb $0

  ; LOOP_BODY
  mov $2,$0
  sub $0,1
  add $2,$0
  bin $2,$0
  add $1,$2

lpe

; POST_LOOP
sub $1,2
mov $0,$1

*/

IncrementalEvaluator::IncrementalEvaluator(Interpreter& interpreter)
    : interpreter(interpreter), arg(-1), previous_loop_count(0) {}

bool IncrementalEvaluator::init(const Program& program) {
  arg = -1;
  previous_loop_count = 0;
  pre_loop.ops.clear();
  loop_body.ops.clear();
  post_loop.ops.clear();
  int64_t phase = 0;
  for (auto& op : program.ops) {
    if (op.type == Operation::Type::NOP) {
      continue;
    }
    if (op.type == Operation::Type::LPB) {
      phase = 1;
      continue;
    }
    if (op.type == Operation::Type::LPE) {
      phase = 2;
      continue;
    }
    switch (phase) {
      case 0:
        pre_loop.ops.push_back(op);
        break;
      case 1:
        loop_body.ops.push_back(op);
        break;
      case 2:
        post_loop.ops.push_back(op);
        break;
      default:
        break;
    }
  }
  return false;
}

Number IncrementalEvaluator::next() {
  arg++;
  // std::cout << "\n==== EVAL N=" << arg << std::endl << std::endl;

  // run init
  init_state.clear();
  init_state.set(0, arg);
  run(pre_loop, init_state);

  // check loop count
  int64_t loop_count = init_state.get(0).asInt();
  if (arg > 0 && loop_count != previous_loop_count + 1) {
    throw std::runtime_error("unexpected loop count: " +
                             std::to_string(loop_count));
  }
  previous_loop_count = loop_count;

  // set loop count / state
  if (arg == 0) {
    loop_state = init_state;
  } else {
    loop_state.set(0, loop_count);
    loop_count = 1;
  }

  while (loop_count-- > 0) {
    run(loop_body, loop_state);
  }

  auto final_state = loop_state;
  run(post_loop, final_state);
  return final_state.get(0);
}

void IncrementalEvaluator::run(const Program& p, Memory& state) {
  // std::cout << "CURRENT MEMORY: " << state << std::endl;
  // ProgramUtil::print(p, std::cout);
  interpreter.run(p, state);
  // std::cout << std::endl;
}
