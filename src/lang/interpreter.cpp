#include "lang/interpreter.hpp"

#include <array>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>

#include "sys/setup.hpp"
#include "lang/number.hpp"
#include "lang/parser.hpp"
#include "lang/program.hpp"
#include "lang/program_util.hpp"
#include "lang/semantics.hpp"
#include "oeis/oeis_sequence.hpp"
#include "sys/log.hpp"

#ifdef _WIN64
#include <io.h>
#else
#include <unistd.h>
#endif

using MemStack = std::stack<Memory>;
using IntStack = std::stack<int64_t>;
using NumStack = std::stack<Number>;
using SizeStack = std::stack<size_t>;

Interpreter::Interpreter(const Settings& settings)
    : settings(settings),
      is_debug(Log::get().level == Log::Level::DEBUG),
      has_memory(true),
      num_memory_checks(0) {}

Number Interpreter::calc(const Operation::Type type, const Number& target,
                         const Number& source) {
  switch (type) {
    case Operation::Type::MOV: {
      return source;
    }
    case Operation::Type::ADD: {
      return Semantics::add(target, source);
    }
    case Operation::Type::SUB: {
      return Semantics::sub(target, source);
    }
    case Operation::Type::TRN: {
      return Semantics::trn(target, source);
    }
    case Operation::Type::MUL: {
      return Semantics::mul(target, source);
    }
    case Operation::Type::DIV: {
      return Semantics::div(target, source);
    }
    case Operation::Type::DIF: {
      return Semantics::dif(target, source);
    }
    case Operation::Type::MOD: {
      return Semantics::mod(target, source);
    }
    case Operation::Type::POW: {
      return Semantics::pow(target, source);
    }
    case Operation::Type::GCD: {
      return Semantics::gcd(target, source);
    }
    case Operation::Type::BIN: {
      return Semantics::bin(target, source);
    }
    case Operation::Type::CMP: {
      return Semantics::cmp(target, source);
    }
    case Operation::Type::MIN: {
      return Semantics::min(target, source);
    }
    case Operation::Type::MAX: {
      return Semantics::max(target, source);
    }
    case Operation::Type::NOP:
    case Operation::Type::DBG:
    case Operation::Type::LPB:
    case Operation::Type::LPE:
    case Operation::Type::CLR:
    case Operation::Type::SEQ:
      Log::get().error(
          "non-arithmetic operation: " + Operation::Metadata::get(type).name,
          true);
      break;
  }
  return 0;
}

bool needsFragments(const Program& p) {
  for (const auto& op : p.ops) {
    if (op.type == Operation::Type::LPB &&
        op.source != Operand(Operand::Type::CONSTANT, Number::ONE)) {
      return true;
    }
  }
  return false;
}

size_t Interpreter::run(const Program& p, Memory& mem) {
  // check for empty program
  if (p.ops.empty()) {
    return 0;
  }

  // define stacks
  SizeStack loop_stack;
  NumStack counter_stack;
  IntStack frag_length_stack;
  MemStack mem_stack;
  MemStack frag_stack;

  size_t cycles = 0;
  const size_t max_cycles = getMaxCycles();
  const bool needs_frags = needsFragments(p);
  const size_t num_ops = p.ops.size();
  Memory old_mem, frag;
  size_t pc;
  Number source, target, counter;
  int64_t start, length, length2;
  Operation lpb;

  // start program execution
  pc = 0;
  while (pc < num_ops) {
    if (is_debug) {
      old_mem = mem;
    }

    auto& op = p.ops[pc];
    size_t pc_next = pc + 1;

    switch (op.type) {
      case Operation::Type::NOP: {
        break;
      }
      case Operation::Type::LPB: {
        if (loop_stack.size() >= 100) {  // magic number
          throw std::runtime_error("Maximum stack size exceeded: " +
                                   std::to_string(loop_stack.size()));
        }
        loop_stack.push(pc);
        mem_stack.push(mem);
        if (needs_frags) {
          length = get(op.source, mem).asInt();
          start = get(op.target, mem, true).asInt();
          if (length > settings.max_memory && settings.max_memory >= 0) {
            throw std::runtime_error("Maximum memory exceeded: " +
                                     std::to_string(length));
          }
          frag = mem.fragment(start, length);
          frag_stack.push(frag);
          frag_length_stack.push(length);
        } else {
          counter = get(op.target, mem, false);
          counter_stack.push(counter);
        }
        break;
      }
      case Operation::Type::LPE: {
        lpb = p.ops[loop_stack.top()];
        if (needs_frags) {
          start = get(lpb.target, mem, true).asInt();
          length2 = get(lpb.source, mem).asInt();
          length = std::min(frag_length_stack.top(), length2);
          frag = mem.fragment(start, length);
          if (frag.is_less(frag_stack.top(), length, true)) {
            pc_next = loop_stack.top() + 1;  // jump back to begin
            mem_stack.top() = mem;
            frag_stack.top() = frag;
            frag_length_stack.top() = length;
          } else {
            mem = mem_stack.top();
            mem_stack.pop();
            loop_stack.pop();
            frag_stack.pop();
            frag_length_stack.pop();
          }
        } else {
          counter = get(lpb.target, mem, false);
          if (Number::MINUS_ONE < counter && counter < counter_stack.top()) {
            pc_next = loop_stack.top() + 1;  // jump back to begin
            mem_stack.top() = mem;
            counter_stack.top() = counter;
          } else {
            mem = mem_stack.top();
            mem_stack.pop();
            loop_stack.pop();
            counter_stack.pop();
          }
        }
        break;
      }
      case Operation::Type::SEQ: {
        target = get(op.target, mem);
        source = get(op.source, mem);
        auto result = call(source.asInt(), target);
        set(op.target, result.first, mem, op);
        cycles += result.second;
        break;
      }
      case Operation::Type::CLR: {
        length = get(op.source, mem).asInt();
        start = get(op.target, mem, true).asInt();
        if (length > 0) {
          mem.clear(start, length);
        }
        break;
      }
      case Operation::Type::DBG: {
        std::cout << mem << std::endl;
        break;
      }
      default: {
        target = get(op.target, mem);
        if (Operation::Metadata::get(op.type).num_operands == 2) {
          source = get(op.source, mem);
        }
        set(op.target, calc(op.type, target, source), mem, op);
        break;
      }
    }
    pc = pc_next;

    // the rest of the logic should be ommitted for nops
    if (op.type == Operation::Type::NOP) {
      continue;
    }

    // count execution steps
    ++cycles;

    // print debug information
    if (is_debug) {
      std::stringstream buf;
      buf << "Executing ";
      ProgramUtil::print(op, buf);
      buf << " " << old_mem << " => " << mem;
      Log::get().debug(buf.str());
    }

    // check resource constraints
    if (cycles > max_cycles) {
      throw std::runtime_error(
          "Exceeded maximum number of steps (" + std::to_string(max_cycles) +
          "); last operation: " + ProgramUtil::operationToString(op));
    }
    if (static_cast<int64_t>(mem.approximate_size()) > settings.max_memory &&
        settings.max_memory >= 0) {
      throw std::runtime_error(
          "Maximum memory exceeded: " + std::to_string(mem.approximate_size()) +
          "; last operation: " + ProgramUtil::operationToString(op));
    }

    // check for external interrupt
    if (Signals::HALT) {
      throw std::runtime_error("interpreter interrupted by halt signal");
    }
  }

  if (loop_stack.size() + counter_stack.size() + mem_stack.size() +
      frag_stack.size() + frag_length_stack.size()) {
    throw std::runtime_error("execution error");
  }
  if (is_debug) {
    Log::get().debug("Finished execution after " + std::to_string(cycles) +
                     " cycles");
  }
  return cycles;
}

size_t Interpreter::run(const Program& p, Memory& mem, int64_t id) {
  size_t result;
  if (id >= 0) {
    running_programs.insert(id);
  }
  try {
    result = run(p, mem);
  } catch (...) {
    if (id >= 0) {
      running_programs.erase(id);
    }
    std::rethrow_exception(std::current_exception());
  }
  if (id >= 0) {
    running_programs.erase(id);
  }
  return result;
}

Number Interpreter::get(const Operand& a, const Memory& mem,
                        bool get_address) const {
  switch (a.type) {
    case Operand::Type::CONSTANT: {
      if (get_address) {
        throw std::runtime_error("Cannot get address of a constant");
      }
      return a.value;
    }
    case Operand::Type::DIRECT: {
      return get_address ? a.value : mem.get(a.value.asInt());
    }
    case Operand::Type::INDIRECT: {
      return get_address ? mem.get(a.value.asInt())
                         : mem.get(mem.get(a.value.asInt()).asInt());
    }
  }
  return {};
}

void Interpreter::set(const Operand& a, const Number& v, Memory& mem,
                      const Operation& last_op) const {
  int64_t index = 0;
  switch (a.type) {
    case Operand::Type::CONSTANT:
      throw std::runtime_error("Cannot set value of a constant");
      break;
    case Operand::Type::DIRECT:
      index = a.value.asInt();
      break;
    case Operand::Type::INDIRECT:
      index = mem.get(a.value.asInt()).asInt();
      break;
  }
  if (index > settings.max_memory && settings.max_memory >= 0) {
    throw std::runtime_error(
        "Maximum memory exceeded: " + std::to_string(index) +
        "; last operation: " + ProgramUtil::operationToString(last_op));
  }
  if (v == Number::INF) {
    throw std::runtime_error(
        "Overflow in cell $" + std::to_string(index) +
        "; last operation: " + ProgramUtil::operationToString(last_op));
  }
  mem.set(index, v);
}

std::pair<Number, size_t> Interpreter::call(int64_t id, const Number& arg) {
  if (arg < 0) {
    throw std::runtime_error("seq using negative argument: " +
                             std::to_string(id));
  }

  // check if already cached
  std::pair<int64_t, Number> key(id, arg);
  auto it = terms_cache.find(key);
  if (it != terms_cache.end()) {
    return it->second;
  }

  // check if program exists
  auto& call_program = getProgram(id);

  // check for recursive calls
  if (running_programs.find(id) != running_programs.end()) {
    throw std::runtime_error("Recursion detected: " +
                             OeisSequence(id).id_str());
  }

  // evaluate program
  std::pair<Number, size_t> result;
  running_programs.insert(id);
  Memory tmp;
  tmp.set(Program::INPUT_CELL, arg);
  try {
    result.second = run(call_program, tmp);
    result.first = tmp.get(Program::OUTPUT_CELL);
    running_programs.erase(id);
  } catch (...) {
    running_programs.erase(id);
    std::rethrow_exception(std::current_exception());
  }

  // add to cache if there is memory available
  if (++num_memory_checks % 10000 == 0) {
    has_memory = Setup::hasMemory();
  }
  if (has_memory || terms_cache.size() < 10000) {  // magic number
    terms_cache[key] = result;
  }
  return result;
}

const Program& Interpreter::getProgram(int64_t id) {
  if (missing_programs.find(id) != missing_programs.end()) {
    throw std::runtime_error("Program not found: " + OeisSequence(id).id_str());
  }
  if (program_cache.find(id) == program_cache.end()) {
    Parser parser;
    auto path = OeisSequence(id).getProgramPath();
    try {
      program_cache[id] = parser.parse(path);
    } catch (...) {
      missing_programs.insert(id);
      std::rethrow_exception(std::current_exception());
    }
  }
  return program_cache[id];
}

size_t Interpreter::getMaxCycles() const {
  return (settings.max_cycles >= 0) ? settings.max_cycles
                                    : std::numeric_limits<size_t>::max();
}

void Interpreter::clearCaches() {
  missing_programs.clear();
  program_cache.clear();
  terms_cache.clear();
}
