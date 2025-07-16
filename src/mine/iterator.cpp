#include "mine/iterator.hpp"

#include <iostream>

#include "eval/semantics.hpp"
#include "lang/program_util.hpp"

const Operand Iterator::CONSTANT_ZERO(Operand::Type::CONSTANT, 0);
const Operand Iterator::CONSTANT_ONE(Operand::Type::CONSTANT, 1);
const Operand Iterator::DIRECT_ZERO(Operand::Type::DIRECT, 0);
const Operand Iterator::DIRECT_ONE(Operand::Type::DIRECT, 1);

const Operand Iterator::SMALLEST_SOURCE = CONSTANT_ZERO;
const Operand Iterator::SMALLEST_TARGET = DIRECT_ZERO;
const Operation Iterator::SMALLEST_OPERATION(
    Operation::Type::MOV, DIRECT_ONE, CONSTANT_ZERO);  // never override $0

bool Iterator::inc(Operand& o, bool direct) {
  const int64_t v = o.value.asInt();
  if (v < 2 || v * 4 < static_cast<int64_t>(size)) {
    o.value = Semantics::add(o.value, Number::ONE);
    return true;
  }
  switch (o.type) {
    case Operand::Type::CONSTANT:
      if (direct) {
        o = Operand(Operand::Type::DIRECT, 0);
        return true;
      } else {
        return false;
      }
    case Operand::Type::DIRECT:
    case Operand::Type::INDIRECT:  // we exclude indirect memory access
      return false;
  }
  return false;
}

bool Iterator::inc(Operation& op) {
  // cannot increase anymore?
  if (op.type == Operation::Type::LPE) {
    return false;
  }

  // try to increase source operand
  if (inc(op.source, op.type != Operation::Type::LPB)) {
    return true;
  }
  op.source = SMALLEST_SOURCE;

  // try to increase target operand
  if (inc(op.target, true)) {
    return true;
  }
  op.target = SMALLEST_TARGET;

  // try to increase type
  switch (op.type) {
    case Operation::Type::MOV:
      op.type = Operation::Type::ADD;
      return true;

    case Operation::Type::ADD:
      op.type = Operation::Type::SUB;
      return true;

    case Operation::Type::SUB:
      op.type = Operation::Type::TRN;
      return true;

    case Operation::Type::TRN:
      op.type = Operation::Type::MUL;
      return true;

    case Operation::Type::MUL:
      op.type = Operation::Type::DIV;
      return true;

    case Operation::Type::DIV:
      op.type = Operation::Type::DIF;
      return true;

    case Operation::Type::DIF:
      op.type = Operation::Type::DIR;
      return true;

    case Operation::Type::DIR:
      op.type = Operation::Type::MOD;
      return true;

    case Operation::Type::MOD:
      op.type = Operation::Type::POW;
      return true;

    case Operation::Type::POW:
      op.type = Operation::Type::GCD;
      return true;

    case Operation::Type::GCD:
      op.type = Operation::Type::LEX;
      return true;

    case Operation::Type::LEX:
      op.type = Operation::Type::BIN;
      return true;

    case Operation::Type::BIN:
      op.type = Operation::Type::DGS;
      return true;

    case Operation::Type::DGS:
      op.type = Operation::Type::DGR;
      return true;

    case Operation::Type::DGR:
      op.type = Operation::Type::EQU;
      return true;

    case Operation::Type::EQU:
      op.type = Operation::Type::NEQ;
      return true;

    case Operation::Type::NEQ:
      op.type = Operation::Type::LPB;
      return true;

    case Operation::Type::FAC:      // skipped
    case Operation::Type::LOG:      // skipped
    case Operation::Type::NRT:      // skipped
    case Operation::Type::LEQ:      // skipped
    case Operation::Type::GEQ:      // skipped
    case Operation::Type::MIN:      // skipped
    case Operation::Type::MAX:      // skipped
    case Operation::Type::BAN:      // skipped
    case Operation::Type::BOR:      // skipped
    case Operation::Type::BXO:      // skipped
    case Operation::Type::NOP:      // skipped
    case Operation::Type::DBG:      // skipped
    case Operation::Type::CLR:      // skipped
    case Operation::Type::SEQ:      // skipped
    case Operation::Type::PRG:      // skipped
    case Operation::Type::__COUNT:  // skipped

    case Operation::Type::LPB:
      op.type = Operation::Type::LPE;
      return true;

    case Operation::Type::LPE:
      return false;
  }
  return false;
}

bool Iterator::supportsOperationType(Operation::Type t) {
  return t != Operation::Type::LOG && t != Operation::Type::NRT &&
         t != Operation::Type::LEQ && t != Operation::Type::GEQ &&
         t != Operation::Type::MIN && t != Operation::Type::MAX &&
         t != Operation::Type::BAN && t != Operation::Type::BOR &&
         t != Operation::Type::BXO && t != Operation::Type::NOP &&
         t != Operation::Type::DBG && t != Operation::Type::CLR &&
         t != Operation::Type::SEQ && t != Operation::Type::PRG;
}

bool Iterator::incWithSkip(Operation& op) {
  do {
    if (!inc(op)) {
      return false;
    }
  } while (shouldSkip(op));
  return true;
}

bool Iterator::shouldSkip(const Operation& op) {
  if (ProgramUtil::isNop(op)) {
    return true;
  }
  // check for trivial operations that can be expressed in a simpler way
  if (op.target == op.source &&
      (op.type == Operation::Type::ADD || op.type == Operation::Type::SUB ||
       op.type == Operation::Type::TRN || op.type == Operation::Type::MUL ||
       op.type == Operation::Type::DIV || op.type == Operation::Type::DIF ||
       op.type == Operation::Type::DIR || op.type == Operation::Type::MOD ||
       op.type == Operation::Type::GCD || op.type == Operation::Type::LEX ||
       op.type == Operation::Type::BIN || op.type == Operation::Type::EQU ||
       op.type == Operation::Type::NEQ)) {
    return true;
  }
  if (op.source == CONSTANT_ZERO &&
      (op.type == Operation::Type::MUL || op.type == Operation::Type::DIV ||
       op.type == Operation::Type::DIF || op.type == Operation::Type::DIR ||
       op.type == Operation::Type::MOD || op.type == Operation::Type::POW ||
       op.type == Operation::Type::GCD || op.type == Operation::Type::LEX ||
       op.type == Operation::Type::BIN || op.type == Operation::Type::LPB)) {
    return true;
  }
  if (op.source == CONSTANT_ONE &&
      (op.type == Operation::Type::MOD || op.type == Operation::Type::POW ||
       op.type == Operation::Type::GCD || op.type == Operation::Type::LEX ||
       op.type == Operation::Type::BIN)) {
    return true;
  }

  return false;
}

Program Iterator::next() {
  while (true) {
    doNext();
    try {
      ProgramUtil::validate(program);
      break;
    } catch (const std::exception&) {
      //      std::cout << "BEGIN IGNORE" << std::endl;
      //      ProgramUtil::print( program, std::cout );
      //      std::cout << "END IGNORE\n" << std::endl;

      // ignore invalid programs
      skipped++;
    }
  }
  return program;
}

void Iterator::doNext() {
  int64_t i = size;
  bool increased = false;
  while (--i >= 0) {
    if (incWithSkip(program.ops[i])) {
      increased = true;

      // begin avoid empty loops
      if (program.ops[i].type == Operation::Type::LPB && i + 3 > size) {
        program.ops[i] = Operation(Operation::Type::LPE);
      }
      if (program.ops[i].type == Operation::Type::LPE && i > 0 &&
          program.ops[i - 1].type == Operation::Type::LPB) {
        increased = false;
      }
      // end avoid empty loops

      // begin avoid lpe if there is no open loop
      if (program.ops[i].type == Operation::Type::LPE) {
        int64_t open_loops = 0;
        for (int64_t j = 0; j < i; j++) {
          if (program.ops[j].type == Operation::Type::LPB) {
            open_loops++;
          }
          if (program.ops[j].type == Operation::Type::LPE) {
            open_loops--;
          }
        }
        if (open_loops <= 0) {
          increased = false;
        }
      }
      // end avoid lpe if there is no open loop
    }
    if (increased) {
      break;
    }
    program.ops[i] = SMALLEST_OPERATION;
  }
  if (!increased) {
    program.ops.insert(program.ops.begin(), SMALLEST_OPERATION);
    size = program.ops.size();
  }

  // begin avoid open loops
  int64_t open_loops = 0;
  for (const auto& op : program.ops) {
    if (op.type == Operation::Type::LPB) {
      open_loops++;
    }
    if (op.type == Operation::Type::LPE) {
      open_loops--;
    }
  }
  i = size;
  while (open_loops > 0 && --i >= 0) {
    if (program.ops[i].type != Operation::Type::LPE) {
      if (program.ops[i].type == Operation::Type::LPB) {
        open_loops--;
      }
      program.ops[i] = Operation(Operation::Type::LPE);
      open_loops--;
    }
  }
  while (--i >= 0 && program.ops[i].type == Operation::Type::LPB) {
    program.ops[i] = Operation(Operation::Type::LPE);
  }
  // end avoid open loops
}
