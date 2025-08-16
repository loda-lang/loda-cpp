#pragma once

#include <unordered_map>
#include <unordered_set>

#include "eval/memory.hpp"
#include "lang/program_cache.hpp"
#include "sys/util.hpp"

class Interpreter {
 public:
  static const std::string ERROR_SEQ_USING_INVALID_ARG;

  explicit Interpreter(const Settings &settings);

  static Number calc(const Operation::Type type, const Number &target,
                     const Number &source);

  size_t run(const Program &p, Memory &mem);

  size_t run(const Program &p, Memory &mem, int64_t id);

  size_t getMaxCycles() const;

  void clearCaches();

  ProgramCache program_cache;

 private:
  Number get(const Operand &a, const Memory &mem,
             bool get_address = false) const;

  void set(const Operand &a, const Number &v, Memory &mem,
           const Operation &last_op) const;

  std::pair<Number, size_t> callSeq(int64_t id, const Number &arg);

  size_t callPrg(int64_t id, int64_t start, Memory &mem);

  const Settings &settings;

  const bool is_debug;
  bool has_memory;
  size_t num_memory_checks;

  struct UIDNumberPairHasher {
    std::size_t operator()(const std::pair<UID, Number> &p) const {
      return (std::hash<UID>()(p.first) << 32) ^ p.second.hash();
    }
  };

  std::unordered_set<UID> running_programs;
  std::unordered_map<std::pair<UID, Number>, std::pair<Number, size_t>,
                     UIDNumberPairHasher>
      terms_cache;
};
