#include "lang/program.hpp"

#include <stdexcept>

const std::array<Operation::Type, 37> Operation::Types = {
    Operation::Type::NOP, Operation::Type::MOV, Operation::Type::ADD,
    Operation::Type::SUB, Operation::Type::TRN, Operation::Type::MUL,
    Operation::Type::DIV, Operation::Type::DIF, Operation::Type::DIR,
    Operation::Type::MOD, Operation::Type::POW, Operation::Type::GCD,
    Operation::Type::LEX, Operation::Type::BIN, Operation::Type::FAC,
    Operation::Type::LOG, Operation::Type::NRT, Operation::Type::DGS,
    Operation::Type::DGR, Operation::Type::EQU, Operation::Type::NEQ,
    Operation::Type::LEQ, Operation::Type::GEQ, Operation::Type::MIN,
    Operation::Type::MAX, Operation::Type::BAN, Operation::Type::BOR,
    Operation::Type::BXO, Operation::Type::LPB, Operation::Type::LPE,
    Operation::Type::CLR, Operation::Type::FIL, Operation::Type::ROL,
    Operation::Type::ROR, Operation::Type::SEQ, Operation::Type::PRG,
    Operation::Type::DBG};

const Operation::Metadata& Operation::Metadata::get(Type t) {
  static Operation::Metadata nop{
      Operation::Type::NOP, "nop", 0, 0, false, false, false};
  static Operation::Metadata mov{
      Operation::Type::MOV, "mov", 1, 2, true, false, true};
  static Operation::Metadata add{
      Operation::Type::ADD, "add", 2, 2, true, true, true};
  static Operation::Metadata sub{
      Operation::Type::SUB, "sub", 3, 2, true, true, true};
  static Operation::Metadata trn{
      Operation::Type::TRN, "trn", 4, 2, true, true, true};
  static Operation::Metadata mul{
      Operation::Type::MUL, "mul", 5, 2, true, true, true};
  static Operation::Metadata div{
      Operation::Type::DIV, "div", 6, 2, true, true, true};
  static Operation::Metadata dif{
      Operation::Type::DIF, "dif", 7, 2, true, true, true};
  static Operation::Metadata dir{
      Operation::Type::DIR, "dir", 8, 2, true, true, true};
  static Operation::Metadata mod{
      Operation::Type::MOD, "mod", 9, 2, true, true, true};
  static Operation::Metadata pow{
      Operation::Type::POW, "pow", 10, 2, true, true, true};
  static Operation::Metadata gcd{
      Operation::Type::GCD, "gcd", 11, 2, true, true, true};
  static Operation::Metadata lex{
      Operation::Type::LEX, "lex", 12, 2, true, true, true};
  static Operation::Metadata bin{
      Operation::Type::BIN, "bin", 13, 2, true, true, true};
  static Operation::Metadata fac{
      Operation::Type::FAC, "fac", 14, 2, true, true, true};
  static Operation::Metadata log{
      Operation::Type::LOG, "log", 15, 2, true, true, true};
  static Operation::Metadata nrt{
      Operation::Type::NRT, "nrt", 16, 2, true, true, true};
  static Operation::Metadata dgs{
      Operation::Type::DGS, "dgs", 17, 2, true, true, true};
  static Operation::Metadata dgr{
      Operation::Type::DGR, "dgr", 18, 2, true, true, true};
  static Operation::Metadata equ{
      Operation::Type::EQU, "equ", 19, 2, true, true, true};
  static Operation::Metadata neq{
      Operation::Type::NEQ, "neq", 20, 2, true, true, true};
  static Operation::Metadata leq{
      Operation::Type::LEQ, "leq", 21, 2, true, true, true};
  static Operation::Metadata geq{
      Operation::Type::GEQ, "geq", 22, 2, true, true, true};
  static Operation::Metadata min{
      Operation::Type::MIN, "min", 23, 2, true, true, true};
  static Operation::Metadata max{
      Operation::Type::MAX, "max", 24, 2, true, true, true};
  static Operation::Metadata ban{
      Operation::Type::BAN, "ban", 25, 2, true, true, true};
  static Operation::Metadata bor{
      Operation::Type::BOR, "bor", 26, 2, true, true, true};
  static Operation::Metadata bxo{
      Operation::Type::BXO, "bxo", 27, 2, true, true, true};
  static Operation::Metadata lpb{
      Operation::Type::LPB, "lpb", 28, 2, true, true, false};
  static Operation::Metadata lpe{
      Operation::Type::LPE, "lpe", 29, 0, true, false, false};
  static Operation::Metadata clr{
      Operation::Type::CLR, "clr", 30, 2, true, false, true};
  static Operation::Metadata fil{
      Operation::Type::FIL, "fil", 31, 2, true, false, true};
  static Operation::Metadata rol{
      Operation::Type::ROL, "rol", 32, 2, true, false, true};
  static Operation::Metadata ror{
      Operation::Type::ROR, "ror", 33, 2, true, false, true};
  static Operation::Metadata seq{
      Operation::Type::SEQ, "seq", 34, 2, true, true, true};
  static Operation::Metadata prg{
      Operation::Type::PRG, "prg", 35, 2, true, true, true};
  static Operation::Metadata dbg{
      Operation::Type::DBG, "dbg", 36, 0, false, false, false};
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
    case Operation::Type::DIR:
      return dir;
    case Operation::Type::MOD:
      return mod;
    case Operation::Type::POW:
      return pow;
    case Operation::Type::GCD:
      return gcd;
    case Operation::Type::LEX:
      return lex;
    case Operation::Type::BIN:
      return bin;
    case Operation::Type::FAC:
      return fac;
    case Operation::Type::LOG:
      return log;
    case Operation::Type::NRT:
      return nrt;
    case Operation::Type::DGS:
      return dgs;
    case Operation::Type::DGR:
      return dgr;
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
    case Operation::Type::FIL:
      return fil;
    case Operation::Type::ROL:
      return rol;
    case Operation::Type::ROR:
      return ror;
    case Operation::Type::SEQ:
      return seq;
    case Operation::Type::PRG:
      return prg;
    case Operation::Type::DBG:
      return dbg;
    case Operation::Type::__COUNT:
      throw std::runtime_error("not an operation type");
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

int64_t Program::getDirective(const std::string& name) const {
  auto d = directives.find(name);
  if (d == directives.end()) {
    throw std::runtime_error("directive not found: " + name);
  }
  return d->second;
}

int64_t Program::getDirective(const std::string& name,
                              int64_t defaultValue) const {
  auto d = directives.find(name);
  return d != directives.end() ? d->second : defaultValue;
}

bool Program::operator==(const Program& p) const { return (ops == p.ops); }

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
