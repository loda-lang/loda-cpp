#include "lang/program.hpp"

#include <stdexcept>

#include "lang/number.hpp"

const std::array<Operation::Type, 20> Operation::Types = {
    Operation::Type::NOP, Operation::Type::MOV, Operation::Type::ADD,
    Operation::Type::SUB, Operation::Type::TRN, Operation::Type::MUL,
    Operation::Type::DIV, Operation::Type::DIF, Operation::Type::MOD,
    Operation::Type::POW, Operation::Type::GCD, Operation::Type::BIN,
    Operation::Type::EQU, Operation::Type::NEQ, Operation::Type::MIN,
    Operation::Type::MAX, Operation::Type::LPB, Operation::Type::LPE,
    Operation::Type::CLR, Operation::Type::SEQ, Operation::Type::DBG,
};

const Operation::Metadata& Operation::Metadata::get(Type t) {
  static Operation::Metadata nop{
      Operation::Type::NOP, "nop", 'n', 0, false, false, false};
  static Operation::Metadata mov{
      Operation::Type::MOV, "mov", 'm', 2, true, false, true};
  static Operation::Metadata add{
      Operation::Type::ADD, "add", 'a', 2, true, true, true};
  static Operation::Metadata sub{
      Operation::Type::SUB, "sub", 's', 2, true, true, true};
  static Operation::Metadata trn{
      Operation::Type::TRN, "trn", 't', 2, true, true, true};
  static Operation::Metadata mul{
      Operation::Type::MUL, "mul", 'u', 2, true, true, true};
  static Operation::Metadata div{
      Operation::Type::DIV, "div", 'd', 2, true, true, true};
  static Operation::Metadata dif{
      Operation::Type::DIF, "dif", 'i', 2, true, true, true};
  static Operation::Metadata mod{
      Operation::Type::MOD, "mod", 'o', 2, true, true, true};
  static Operation::Metadata pow{
      Operation::Type::POW, "pow", 'p', 2, true, true, true};
  static Operation::Metadata gcd{
      Operation::Type::GCD, "gcd", 'g', 2, true, true, true};
  static Operation::Metadata bin{
      Operation::Type::BIN, "bin", 'b', 2, true, true, true};
  static Operation::Metadata equ{
      Operation::Type::EQU, "equ", 'f', 2, true, true, true};
  static Operation::Metadata neq{
      Operation::Type::EQU, "neq", 'g', 2, true, true, true};
  static Operation::Metadata min{
      Operation::Type::MIN, "min", 'v', 2, true, true, true};
  static Operation::Metadata max{
      Operation::Type::MAX, "max", 'w', 2, true, true, true};
  static Operation::Metadata lpb{
      Operation::Type::LPB, "lpb", 'l', 2, true, true, false};
  static Operation::Metadata lpe{
      Operation::Type::LPE, "lpe", 'e', 0, true, false, false};
  static Operation::Metadata clr{
      Operation::Type::CLR, "clr", 'r', 2, true, false, true};
  static Operation::Metadata seq{
      Operation::Type::SEQ, "seq", 'q', 2, true, true, true};
  static Operation::Metadata dbg{
      Operation::Type::DBG, "dbg", 'x', 0, false, false, false};
  switch (t) {
    case Operation::Type::NOP:
      return nop;
    case Operation::Type::MOV:
      return mov;
    case Operation::Type::ADD:
      return add;
    case Operation::Type::SUB:
      return sub;
    case Operation::Type::TRN:
      return trn;
    case Operation::Type::MUL:
      return mul;
    case Operation::Type::DIV:
      return div;
    case Operation::Type::DIF:
      return dif;
    case Operation::Type::MOD:
      return mod;
    case Operation::Type::POW:
      return pow;
    case Operation::Type::GCD:
      return gcd;
    case Operation::Type::BIN:
      return bin;
    case Operation::Type::EQU:
      return equ;
    case Operation::Type::NEQ:
      return neq;
    case Operation::Type::MIN:
      return min;
    case Operation::Type::MAX:
      return max;
    case Operation::Type::LPB:
      return lpb;
    case Operation::Type::LPE:
      return lpe;
    case Operation::Type::CLR:
      return clr;
    case Operation::Type::SEQ:
      return seq;
    case Operation::Type::DBG:
      return dbg;
  }
  return nop;
}

const Operation::Metadata& Operation::Metadata::get(const std::string& name) {
  for (auto t : Operation::Types) {
    auto& m = get(t);
    if (m.name == name) {
      return m;
    }
  }
  if (name == "cmp") {  // backwards compatibility
    return get(Operation::Type::EQU);
  }
  throw std::runtime_error("invalid operation: " + name);
}

void Program::push_front(Operation::Type t, Operand::Type tt, const Number& tv,
                         Operand::Type st, const Number& sv) {
  ops.insert(ops.begin(), Operation(t, Operand(tt, tv), Operand(st, sv)));
}

void Program::push_back(Operation::Type t, Operand::Type tt, const Number& tv,
                        Operand::Type st, const Number& sv) {
  ops.insert(ops.end(), Operation(t, Operand(tt, tv), Operand(st, sv)));
}

bool Program::operator==(const Program& p) const {
  if (p.ops.size() != ops.size()) {
    return false;
  }
  for (size_t i = 0; i < ops.size(); ++i) {
    if (!(ops[i] == p.ops[i])) {
      return false;
    }
  }
  return true;
}

bool Program::operator!=(const Program& p) const { return !(*this == p); }

bool Program::operator<(const Program& p) const {
  if (ops.size() < p.ops.size()) {
    return true;
  } else if (ops.size() > p.ops.size()) {
    return false;
  }
  for (size_t i = 0; i < ops.size(); i++) {
    if (ops[i] < p.ops[i]) {
      return true;
    } else if (p.ops[i] < ops[i]) {
      return false;
    }
  }
  return false;  // equal
}
