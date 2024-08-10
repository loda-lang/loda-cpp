#include "lang/program.hpp"

#include <stdexcept>

#include "lang/number.hpp"

const std::array<Operation::Type, 27> Operation::Types = {
    Operation::Type::NOP, Operation::Type::MOV, Operation::Type::ADD,
    Operation::Type::SUB, Operation::Type::TRN, Operation::Type::MUL,
    Operation::Type::DIV, Operation::Type::DIF, Operation::Type::MOD,
    Operation::Type::POW, Operation::Type::GCD, Operation::Type::BIN,
    Operation::Type::EQU, Operation::Type::NEQ, Operation::Type::LEQ,
    Operation::Type::GEQ, Operation::Type::MIN, Operation::Type::MAX,
    Operation::Type::BAN, Operation::Type::BOR, Operation::Type::BXO,
    Operation::Type::LPB, Operation::Type::LPE, Operation::Type::CLR,
    Operation::Type::SOR, Operation::Type::SEQ, Operation::Type::DBG,
};

const Operation::Metadata& Operation::Metadata::get(Type t) {
  static Operation::Metadata nop{
      Operation::Type::NOP, "nop", 0, false, false, false};
  static Operation::Metadata mov{
      Operation::Type::MOV, "mov", 2, true, false, true};
  static Operation::Metadata add{
      Operation::Type::ADD, "add", 2, true, true, true};
  static Operation::Metadata sub{
      Operation::Type::SUB, "sub", 2, true, true, true};
  static Operation::Metadata trn{
      Operation::Type::TRN, "trn", 2, true, true, true};
  static Operation::Metadata mul{
      Operation::Type::MUL, "mul", 2, true, true, true};
  static Operation::Metadata div{
      Operation::Type::DIV, "div", 2, true, true, true};
  static Operation::Metadata dif{
      Operation::Type::DIF, "dif", 2, true, true, true};
  static Operation::Metadata mod{
      Operation::Type::MOD, "mod", 2, true, true, true};
  static Operation::Metadata pow{
      Operation::Type::POW, "pow", 2, true, true, true};
  static Operation::Metadata gcd{
      Operation::Type::GCD, "gcd", 2, true, true, true};
  static Operation::Metadata bin{
      Operation::Type::BIN, "bin", 2, true, true, true};
  static Operation::Metadata equ{
      Operation::Type::EQU, "equ", 2, true, true, true};
  static Operation::Metadata neq{
      Operation::Type::NEQ, "neq", 2, true, true, true};
  static Operation::Metadata leq{
      Operation::Type::LEQ, "leq", 2, true, true, true};
  static Operation::Metadata geq{
      Operation::Type::GEQ, "geq", 2, true, true, true};
  static Operation::Metadata min{
      Operation::Type::MIN, "min", 2, true, true, true};
  static Operation::Metadata max{
      Operation::Type::MAX, "max", 2, true, true, true};
  static Operation::Metadata ban{
      Operation::Type::BAN, "ban", 2, true, true, true};
  static Operation::Metadata bor{
      Operation::Type::BOR, "bor", 2, true, true, true};
  static Operation::Metadata bxo{
      Operation::Type::BXO, "bxo", 2, true, true, true};
  static Operation::Metadata lpb{
      Operation::Type::LPB, "lpb", 2, true, true, false};
  static Operation::Metadata lpe{
      Operation::Type::LPE, "lpe", 0, true, false, false};
  static Operation::Metadata clr{
      Operation::Type::CLR, "clr", 2, true, false, true};
  static Operation::Metadata sor{
      Operation::Type::SOR, "sor", 2, true, true, true};
  static Operation::Metadata seq{
      Operation::Type::SEQ, "seq", 2, true, true, true};
  static Operation::Metadata dbg{
      Operation::Type::DBG, "dbg", 0, false, false, false};
  static Operation::Metadata __count{
      Operation::Type::__COUNT, "__count", 0, false, false, false};
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
    case Operation::Type::LEQ:
      return leq;
    case Operation::Type::GEQ:
      return geq;
    case Operation::Type::MIN:
      return min;
    case Operation::Type::MAX:
      return max;
    case Operation::Type::BAN:
      return ban;
    case Operation::Type::BOR:
      return bor;
    case Operation::Type::BXO:
      return bxo;
    case Operation::Type::LPB:
      return lpb;
    case Operation::Type::LPE:
      return lpe;
    case Operation::Type::CLR:
      return clr;
    case Operation::Type::SOR:
      return sor;
    case Operation::Type::SEQ:
      return seq;
    case Operation::Type::DBG:
      return dbg;
    case Operation::Type::__COUNT:
      return __count;
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
