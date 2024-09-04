#include "cmd/benchmark.hpp"

#include <fstream>
#include <queue>
#include <sstream>

#include "eval/evaluator.hpp"
#include "eval/evaluator_inc.hpp"
#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "oeis/oeis_sequence.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"

void Benchmark::smokeTest() {
  operations();
  programs();
}

std::string fillString(std::string s, size_t n) {
  while (s.length() < n) {
    s += " ";
  }
  return s;
}

void Benchmark::operations() {
  std::cout << "| Operation |  Time     |" << std::endl;
  std::cout << "|-----------|-----------|" << std::endl;
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
              << Operation::Metadata::get(type).name + "    | " +
                     fillString(buf.str() + "µs", 10) + " |"
              << std::endl;
  }
  std::cout << std::endl;
}

void Benchmark::programs() {
  Setup::setProgramsHome("tests/programs");
  std::cout << "| Sequence | Terms  | Reg Eval | Inc Eval |" << std::endl;
  std::cout << "|----------|--------|----------|----------|" << std::endl;
  program(796, 300);
  program(1041, 300);
  program(1113, 300);
  program(2110, 300);
  program(57552, 300);
  program(79309, 300);
  program(2193, 400);
  program(35856, 500);
  program(1609, 1000);
  program(3411, 1000);
  program(12866, 1000);
  program(45, 2000);
  program(1304, 3000);
  program(5, 5000);
  program(130487, 5000);
  program(30, 500000);
  std::cout << std::endl;
}

void Benchmark::program(size_t id, size_t num_terms) {
  Parser parser;
  const OeisSequence seq(id);
  auto program = parser.parse(seq.getProgramPath());
  auto speed_reg = programEval(program, false, num_terms);
  auto speed_inc = programEval(program, true, num_terms);
  std::cout << "| " << seq.id_str() << "  | "
            << fillString(std::to_string(num_terms), 6) << " | "
            << fillString(speed_reg, 8) << " | " << fillString(speed_inc, 8)
            << " |" << std::endl;
}

std::string Benchmark::programEval(const Program& p, bool use_inc_eval,
                                   size_t num_terms) {
  Settings settings;
  Interpreter interpreter(settings);
  IncrementalEvaluator inc_eval(interpreter);
  if (use_inc_eval && !inc_eval.init(p)) {
    return "-";
  }
  Sequence result;
  Evaluator evaluator(settings, use_inc_eval);
  static const size_t runs = 4;
  auto start_time = std::chrono::steady_clock::now();
  for (size_t i = 0; i < runs; i++) {
    evaluator.eval(p, result, num_terms, true);
    if (result.size() != num_terms) {
      Log::get().error(
          "Unexpected sequence length: " + std::to_string(result.size()), true);
    }
  }
  auto end_time = std::chrono::steady_clock::now();
  double speed = std::chrono::duration_cast<std::chrono::milliseconds>(
                     end_time - start_time)
                     .count() /
                 static_cast<double>(runs * 1000);
  std::stringstream buf;
  buf.setf(std::ios::fixed);
  buf.precision(2);
  buf << speed << "s";
  return buf.str();
}

void Benchmark::findSlow(int64_t num_terms, Operation::Type type) {
  Parser parser;
  Settings settings;
  Interpreter interpreter(settings);
  Evaluator evaluator(settings);
  Sequence seq;
  Program program;
  std::priority_queue<std::pair<int64_t, int64_t> > queue;
  for (size_t id = 0; id < 400000; id++) {
    OeisSequence oeisSeq(id);
    std::ifstream in(oeisSeq.getProgramPath());
    if (!in) {
      continue;
    }
    try {
      program = parser.parse(in);
    } catch (std::exception& e) {
      Log::get().warn("Skipping " + oeisSeq.id_str() + ": " + e.what());
      continue;
    }
    if (type != Operation::Type::NOP && !ProgramUtil::hasOp(program, type)) {
      continue;
    }
    auto start_time = std::chrono::steady_clock::now();
    evaluator.eval(program, seq, num_terms, false);
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);
    Log::get().info(oeisSeq.id_str() + ": " + std::to_string(duration.count()) +
                    "µs");
    queue.push(std::pair<int64_t, int64_t>(duration.count(), oeisSeq.id));
  }
  std::cout << std::endl << "Slowest programs:" << std::endl;
  for (size_t i = 0; i < 20; i++) {
    auto entry = queue.top();
    queue.pop();
    OeisSequence oeisSeq(entry.second);
    std::cout << "[" << oeisSeq.id_str()
              << "](https://loda-lang.org/edit/?oeis=" << entry.second
              << "): " << (entry.first / 1000) << "ms" << std::endl;
  }
}