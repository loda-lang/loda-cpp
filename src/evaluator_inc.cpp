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
    : interpreter(interpreter) {
  reset();
}

void IncrementalEvaluator::reset() {
  loop_counter_cell = 0;
  argument = 0;
  previous_loop_count = 0;
  initialized = false;
  pre_loop.ops.clear();
  loop_body.ops.clear();
  post_loop.ops.clear();
}

bool IncrementalEvaluator::init(const Program& program) {
  reset();
  if (!extractFragments(program)) {
    return false;
  }
  if (!checkPreLoop()) {
    return false;
  }
  if (!checkLoopBody()) {
    return false;
  }
  if (!checkPostLoop()) {
    return false;
  }
  initialized = true;
  return true;
}

Number IncrementalEvaluator::next() {
  // sanity check: must be initialized
  if (!initialized) {
    throw std::runtime_error("incremental evaluator not initialized");
  }
  std::cout << "\n==== EVAL N=" << argument << std::endl << std::endl;

  // execute pre-loop code
  init_state.clear();
  init_state.set(0, argument);
  run(pre_loop, init_state);

  // calculate new loop count
  const int64_t new_loop_count = init_state.get(0).asInt();
  int64_t additional_loops = new_loop_count - previous_loop_count;
  previous_loop_count = new_loop_count;

  // sanity check: loop count cannot be negative
  if (additional_loops < 0) {
    throw std::runtime_error("unexpected loop count: " +
                             std::to_string(additional_loops));
  }

  // update loop state
  if (argument == 0) {
    loop_state = init_state;
  } else {
    loop_state.set(0, new_loop_count);
  }

  // execute loop body
  while (additional_loops-- > 0) {
    run(loop_body, loop_state);
  }

  // execute post-loop code
  auto final_state = loop_state;
  run(post_loop, final_state);

  // prepare next iteration
  argument++;

  // return result of execution
  return final_state.get(0);
}

bool IncrementalEvaluator::extractFragments(const Program& program) {
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
  return true;
}

bool IncrementalEvaluator::checkPreLoop() { return true; }

bool IncrementalEvaluator::checkLoopBody() { return true; }

bool IncrementalEvaluator::checkPostLoop() { return true; }

void IncrementalEvaluator::run(const Program& p, Memory& state) {
  std::cout << "CURRENT MEMORY: " << state << std::endl;
  // ProgramUtil::print(p, std::cout);
  interpreter.run(p, state);
  // std::cout << std::endl;
}
