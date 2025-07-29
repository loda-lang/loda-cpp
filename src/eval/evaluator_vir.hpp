#include "eval/interpreter.hpp"

class VirtualEvaluator {
 public:
  explicit VirtualEvaluator(const Settings &settings);

  bool init(const Program &p);

  std::pair<Number, size_t> eval(const Number &input);

 private:
  Interpreter interpreter;
  Program refactored;
  Memory tmp_memory;
};
