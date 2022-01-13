#include "benchmark.hpp"

#include <sstream>

#include "evaluator.hpp"
#include "oeis_sequence.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "setup.hpp"
#include "util.hpp"

void Benchmark::all() {
  // operations();
  programs();
}

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
    buf.precision(2);
    buf << speed;
    std::cout << "|    "
              << Operation::Metadata::get(type).name + "    | " + buf.str() +
                     "µs |"
              << std::endl;
  }
  std::cout << std::endl;
}

void Benchmark::programs() {
  Setup::setProgramsHome("tests/programs");
  std::cout << "| Sequence | Terms | Time    |" << std::endl;
  std::cout << "|----------|-------|---------|" << std::endl;
  // program(796, 300);
  // program(1041, 300);
  // program(1113, 300);
  // program(2110, 300);
  program(79309, 800);
  // program(2193, 400);
  // program(45, 2000);
  // program(5, 5000);
  // program(30, 500000);
  std::cout << std::endl;
}

void Benchmark::program(size_t id, size_t num_terms) {
  Parser parser;
  Settings settings;
  Sequence result;
  Evaluator evaluator(settings);
  static const size_t runs = 5;
  const OeisSequence seq(id);
  auto program = parser.parse(seq.getProgramPath());
  auto start_time = std::chrono::steady_clock::now();
  for (size_t i = 0; i < runs; i++) {
    evaluator.eval(program, result, num_terms);
    if (result.size() != num_terms) {
      Log::get().error(
          "Unexpected sequence length: " + std::to_string(result.size()), true);
    }
  }
  auto cur_time = std::chrono::steady_clock::now();
  double speed = std::chrono::duration_cast<std::chrono::milliseconds>(
                     cur_time - start_time)
                     .count() /
                 static_cast<double>(runs * 1000);
  std::stringstream buf;
  buf.setf(std::ios::fixed);
  buf.precision(2);
  buf << speed;
  std::cout << "| " << seq.id_str() << "  | " << num_terms << "   | "
            << buf.str() << "s   |" << std::endl;

  std::cout << "result: " << result.back() << std::endl;
}
