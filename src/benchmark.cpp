#include "benchmark.hpp"

#include <sstream>

#include "interpreter.hpp"
#include "program_util.hpp"
#include "util.hpp"

void Benchmark::all() { operations(); }

void Benchmark::operations() {
  std::cout << "| Operation |  Time   |" << std::endl;
  std::cout << "|-----------|---------|" << std::endl;
  std::vector<Number> ops(10000);
  int64_t num_digits;
  std::string str;
  for (Number& n : ops) {
    if (Random::get().gen() % 2) {
      num_digits = (Random::get().gen() % 500) + 1;
    } else {
      num_digits = (Random::get().gen() % 18) + 1;
    }
    str.clear();
    if (Random::get().gen() % 2) {
      str += '-';
    }
    str += '1' + static_cast<char>((Random::get().gen() % 9));
    for (int64_t j = 1; j < num_digits; j++) {
      str += '0' + static_cast<char>((Random::get().gen() % 10));
    }
    n = Number(str);
  }
  for (auto& type : Operation::Types) {
    if (!ProgramUtil::isArithmetic(type)) {
      continue;
    }
    auto start_time = std::chrono::steady_clock::now();
    for (size_t i = 0; i + 1 < ops.size(); i++) {
      try {
        Interpreter::calc(type, ops[i], ops[i + 1]);
      } catch (const std::exception& e) {
        // Log::get().warn( std::string( e.what() ) );
      }
    }
    auto cur_time = std::chrono::steady_clock::now();
    double speed = std::chrono::duration_cast<std::chrono::microseconds>(
                       cur_time - start_time)
                       .count() /
                   static_cast<double>(ops.size());
    std::stringstream buf;
    buf.setf(std::ios::fixed);
    buf.precision(3);
    buf << speed;
    std::cout << "|    "
              << Operation::Metadata::get(type).name + "    | " + buf.str() +
                     "µs |"
              << std::endl;
  }
  std::cout << std::endl;
}
