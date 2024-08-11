#pragma once

#include <array>
#include <vector>

#include "lang/number.hpp"

class Operand {
 public:
  enum class Type { CONSTANT, DIRECT, INDIRECT };

  Operand() : Operand(Type::CONSTANT, 0) {}

  Operand(Type t, const Number &v) : type(t), value(v) {}

  inline bool operator==(const Operand &o) const {
    return (type == o.type) && (value == o.value);
  }

  inline bool operator!=(const Operand &o) const { return !((*this) == o); }

  inline bool operator<(const Operand &o) const {
    if (type != o.type) return type < o.type;
    if (value != o.value) return value < o.value;
    return false;  // equal
  }

  Type type;
  Number value;
};

class Operation {
 public:
  enum class Type {
    NOP,     // no operation
    MOV,     // assignment
    ADD,     // addition
    SUB,     // subtraction
    TRN,     // truncated subtraction
    MUL,     // multiplication
    DIV,     // division
    DIF,     // conditional division
    MOD,     // modulo
    POW,     // power
    GCD,     // greatest common divisor
    BIN,     // binomial coefficient
    DIS,     // digit sum
    DIR,     // digital root
    EQU,     // equality
    NEQ,     // inequality
    LEQ,     // less or equal
    GEQ,     // greater or equal
    MIN,     // minimum
    MAX,     // maximum
    BAN,     // bitwise and
    BOR,     // bitwise or
    BXO,     // bitwise xor
    LPB,     // loop begin
    LPE,     // loop end
    CLR,     // clear
    SOR,     // sort
    SEQ,     // sequence
    DBG,     // debug
    __COUNT  // number of operation types
  };

  static const std::array<Type, 29> Types;

  class Metadata {
   public:
    static const Metadata &get(Type t);

    static const Metadata &get(const std::string &name);

    Type type;
    std::string name;
    size_t num_operands;
    bool is_public;
    bool is_reading_target;
    bool is_writing_target;
  };

  Operation() : Operation(Type::NOP) {}

  explicit Operation(Type y)
      : Operation(y, {Operand::Type::DIRECT, 0}, {Operand::Type::CONSTANT, 0}) {
  }

  Operation(Type y, Operand t, Operand s, const std::string &c = "")
      : type(y), target(t), source(s), comment(c) {}

  inline bool operator==(const Operation &op) const {
    return (type == op.type) && (source == op.source) && (target == op.target);
  }

  inline bool operator!=(const Operation &op) const { return !((*this) == op); }

  inline bool operator<(const Operation &op) const {
    if (type != op.type) return type < op.type;
    if (target != op.target) return target < op.target;
    if (source != op.source) return source < op.source;
    return false;  // equal
  }

  Type type;
  Operand target;
  Operand source;
  std::string comment;
};

class Program {
 public:
  static constexpr int64_t INPUT_CELL = 0;
  static constexpr int64_t OUTPUT_CELL = 0;

  void push_front(Operation::Type t, Operand::Type tt, const Number &tv,
                  Operand::Type st, const Number &sv);

  void push_back(Operation::Type t, Operand::Type tt, const Number &tv,
                 Operand::Type st, const Number &sv);

  bool operator==(const Program &p) const;

  bool operator!=(const Program &p) const;

  bool operator<(const Program &p) const;

  std::vector<Operation> ops;
};
