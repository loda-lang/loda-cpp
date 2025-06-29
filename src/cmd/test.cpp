#include "cmd/test.hpp"

#include <deque>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "eval/evaluator.hpp"
#include "eval/interpreter.hpp"
#include "eval/minimizer.hpp"
#include "eval/optimizer.hpp"
#include "eval/semantics.hpp"
#include "form/formula_gen.hpp"
#include "form/pari.hpp"
#include "form/range_generator.hpp"
#include "lang/comments.hpp"
#include "lang/constants.hpp"
#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "lang/subprogram.hpp"
#include "math/big_number.hpp"
#include "mine/api_client.hpp"
#include "mine/blocks.hpp"
#include "mine/config.hpp"
#include "mine/generator_v1.hpp"
#include "mine/iterator.hpp"
#include "mine/matcher.hpp"
#include "mine/miner.hpp"
#include "mine/stats.hpp"
#include "oeis/oeis_list.hpp"
#include "oeis/oeis_manager.hpp"
#include "sys/file.hpp"
#include "sys/git.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"

Test::Test() {
  settings.max_memory = 100000;  // for ackermann
  settings.max_cycles = 10000000;
  const std::string home = getTmpDir() + "loda" + FILE_SEP;
  ensureDir(home);
  Setup::setLodaHome(home);
  Setup::setMinersConfig(std::string("tests") + FILE_SEP + "config" + FILE_SEP +
                         "test_miners.json");
  Setup::setProgramsHome(std::string("tests") + FILE_SEP + "programs");
}

void Test::all() {
  fast();
  slow();
}

void Test::fast() {
  sequence();
  memory();
  operationMetadata();
  programUtil();
  semantics();
  config();
  steps();
  blocks();
  fold();
  unfold();
  incEval();
  linearMatcher();
  deltaMatcher();
  digitMatcher();
  optimizer();
  checkpoint();
  knownPrograms();
  formula();
  range();
}

void Test::slow() {
  number();
  randomNumber(100);
  ackermann();
  stats();
  apiClient();  // requires API server
  oeisList();
  oeisSeq();
  iterator(100);
  minimizer(100);
  randomRange(100);
  miner();
  memUsage();
}

OeisManager& Test::getManager() {
  if (!manager_ptr) {
    manager_ptr.reset(new OeisManager(settings, getTmpDir() + "stats"));
  }
  return *manager_ptr;
}

void check_num(const Number& m, const std::string& s) {
  if (m.to_string() != s) {
    Log::get().error("Expected " + m.to_string() + " to be " + s, true);
  }
}

void check_inf(const Number& n) { check_num(n, "inf"); }

void check_less(const Number& m, const Number& n) {
  if (!(m < n)) {
    Log::get().error(
        "Expected " + m.to_string() + " to be less than " + n.to_string(),
        true);
  }
}

Number read_num(const std::string& s) {
  auto n = Number(s);
  check_num(n, s);
  return n;
}

void testNumberDigits(int64_t num_digits, bool test_negative) {
  for (char d = '1'; d <= '9'; d++) {
    std::string str;
    if (test_negative) {
      str += '-';
    }
    str += d;
    for (int64_t i = 0; i < num_digits; i++) {
      str += d;
      Number n(str);
      check_num(n, str);
    }
  }
}

void Test::number() {
  Log::get().info("Testing number");
  check_num(Number::ZERO, "0");
  check_num(Number::ONE, "1");
  check_num(Number("1"), "1");
  check_num(Number("2 "), "2");
  check_num(Number(" 3"), "3");
  check_num(Number("-4 "), "-4");
  check_inf(Number::INF);
  check_less(Number::ZERO, Number::ONE);
  check_num(std::numeric_limits<int64_t>::max(),
            std::to_string(std::numeric_limits<int64_t>::max()));
  check_num(std::numeric_limits<int64_t>::min(),
            std::to_string(std::numeric_limits<int64_t>::min()));
  Number o(1);
  o += Number::TWO;
  check_num(o, "3");
  o += Number(-5);
  check_num(o, "-2");
  o *= Number(5);
  check_num(o, "-10");
  o *= Number(-10);
  check_num(o, "100");
  o %= Number(3);
  check_num(o, "1");
  auto m = Number::MAX;
  m += 1;
  check_inf(m);
  m = Number::MIN;
  m += -1;
  check_inf(m);
  m = Number(std::numeric_limits<int64_t>::min());
  m /= Number(-1);
  check_num(m, "9223372036854775808");
  m = Number(std::numeric_limits<int64_t>::min());
  m %= Number(-1);
  check_num(m, "0");
  testNumberDigits(USE_BIG_NUMBER ? (BigNumber::NUM_WORDS * 18) : 18, false);
  testNumberDigits(USE_BIG_NUMBER ? (BigNumber::NUM_WORDS * 18) : 18, true);
}

void Test::randomNumber(size_t tests) {
  Log::get().info("Testing random number");
  std::string str, inv, nines;
  for (size_t i = 0; i < tests; i++) {
    // small number test
    int64_t v = Random::get().gen() / 2, w = Random::get().gen() / 2;
    if (Random::get().gen() % 2) v *= -1;
    if (Random::get().gen() % 2) w *= -1;
    check_num(Number(v), std::to_string(v));
    Number vv(v);
    Number ww(w);
    if (v < w) {
      check_less(vv, ww);
    } else if (w < v) {
      check_less(ww, vv);
    }
    auto xx = vv;
    xx += ww;
    check_num(xx, std::to_string(v + w));
    xx = vv;
    xx *= ww;
    xx /= vv;
    check_num(xx, std::to_string(w));
    xx = vv;
    xx %= ww;
    check_num(xx, std::to_string(v % w));

    // big number test
    if (USE_BIG_NUMBER) {
      const int64_t num_digits =
          (Random::get().gen() % (BigNumber::NUM_WORDS * 18)) + 1;
      char ch;
      str.clear();
      inv.clear();
      nines.clear();
      if (Random::get().gen() % 2) {
        str += '-';
        inv += '-';
        nines += '-';
      }
      ch = static_cast<char>((Random::get().gen() % 9));
      str += '1' + ch;
      inv += '8' - ch;
      nines += '9';
      for (int64_t j = 1; j < num_digits; j++) {
        ch = static_cast<char>((Random::get().gen() % 10));
        str += '0' + ch;
        inv += '9' - ch;
        nines += '9';
      }
      Number n(str);
      check_num(n, str);
      check_num(Number(n), str);
      Number triple1 = n;
      Number triple2 = n;
      Number triple3(3);
      triple1 += n;
      triple1 += n;
      triple2 *= Number(3);
      triple3 *= n;
      check_num(triple1, triple2.to_string());
      check_num(triple1, triple3.to_string());
      if (triple1 != Number::INF) {
        auto t = triple3;
        auto neg = n;
        neg.negate();
        t += neg;
        t += neg;
        check_num(t, n.to_string());
        auto u = triple3;
        u /= Number(3);
        check_num(u, n.to_string());
      }
      if (str.size() > 2) {
        auto smaller = str.substr(0, str.size() - 1);
        Number m(smaller);
        if (str[0] == '-') {
          check_less(n, m);
        } else {
          check_less(m, n);
        }
      }
      Number o(inv);
      o += n;
      check_num(o, nines);
    }
  }
}

void Test::semantics() {
  for (auto& type : Operation::Types) {
    if (!ProgramUtil::isArithmetic(type)) {
      continue;
    }
    auto& meta = Operation::Metadata::get(type);
    std::string test_path = std::string("tests") + FILE_SEP + "semantics" +
                            FILE_SEP + Operation::Metadata::get(type).name +
                            ".csv";
    std::ifstream test_file(test_path);
    if (!test_file.good()) {
      Log::get().error("Test file not found: " + test_path, true);
      continue;
    }
    Log::get().info("Testing " + test_path);
    std::string line, s, t, r;
    Number op1, op2, expected_op, result_op;
    std::getline(test_file, line);  // skip header
    while (std::getline(test_file, line)) {
      if (line.empty() || line[0] == '#') {
        continue;
      }
      std::stringstream ss(line);
      std::getline(ss, s, ',');
      if (meta.num_operands == 2) {
        std::getline(ss, t, ',');
      }
      std::getline(ss, r);
      op1 = read_num(s);
      if (meta.num_operands == 2) {
        op2 = read_num(t);
      }
      expected_op = read_num(r);
      result_op = Interpreter::calc(type, op1, op2);
      if (result_op != expected_op) {
        Log::get().error("Unexpected value for " + meta.name + "(" +
                             op1.to_string() + "," + op2.to_string() +
                             "); expected " + expected_op.to_string() +
                             "; got " + result_op.to_string(),
                         true);
      }
    }
    if (type != Operation::Type::MOV) {
      check_inf(Interpreter::calc(type, Number::INF, 0));
      check_inf(Interpreter::calc(type, Number::INF, 1));
      check_inf(Interpreter::calc(type, Number::INF, -1));
    }
    if (meta.num_operands == 2) {
      check_inf(Interpreter::calc(type, 0, Number::INF));
      check_inf(Interpreter::calc(type, 1, Number::INF));
      check_inf(Interpreter::calc(type, -1, Number::INF));
    }
  }
  if (Semantics::getPowerOf(0, 2) != Number::INF) {
    Log::get().error("Unexpected power-of result", true);
  }
}

void Test::sequence() {
  Log::get().info("Testing sequence");
  Sequence s({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  Sequence t({2, 3, 4, 5, 6, 7, 8, 9});
  auto u = s.subsequence(1, 8);
  if (t != u) {
    Log::get().error("Error comparing subsequence", true);
  }
  if (t.to_string() != "2,3,4,5,6,7,8,9") {
    Log::get().error("Error printing sequence", true);
  }
  if (!s.is_linear(0)) {
    Log::get().error("Sequence should be linear", true);
  }
  Sequence v = s;
  v.push_back(42);
  if (v.is_linear(0)) {
    Log::get().error("Sequence should not be linear", true);
  }
}

void checkMemory(const Memory& mem, int64_t index, const Number& value) {
  if (mem.get(index) != value) {
    Log::get().error("Unexpected memory value at index " +
                         std::to_string(index) +
                         "; expected: " + value.to_string() +
                         "; found: " + mem.get(index).to_string(),
                     true);
  }
}

void checkMemoryString(const std::string& in, const std::string& out) {
  Memory mem(in);
  std::stringstream buf;
  buf << mem;
  auto buf_str = buf.str();
  if (buf_str != out) {
    Log::get().error(
        "Unexpected memory string: " + buf_str + " - expected: " + out, true);
  }
}

void checkMemoryString(const std::string& s) { checkMemoryString(s, s); }

void Test::memory() {
  Log::get().info("Testing memory");

  // get and set
  Memory base;
  int64_t size = 100;
  for (int64_t i = 0; i < size; i++) {
    base.set(i, i);
    checkMemory(base, i, i);
  }
  bool ok;
  try {
    checkMemory(base, -1, 0);
    ok = false;
  } catch (...) {
    ok = true;
  }
  if (!ok) {
    throw std::runtime_error(
        "unexpected behavior for memory access with negative index");
  }
  checkMemory(base, size + 1, 0);

  // parsing and printing
  checkMemoryString("1:1,2:2,6:6,7:7,10:10");
  checkMemoryString("1:9,14:8,37:3");
  checkMemoryString("187:-4131239,3114:98234234234225211,374441:-98234");
  checkMemoryString("37:31,14:8,17:-3,1:9,21:458",
                    "1:9,14:8,17:-3,21:458,37:31");
  checkMemoryString("98:-17,54:99,73:-313,14:-72",
                    "14:-72,54:99,73:-313,98:-17");

  // fragments
  int64_t max_frag_length = 50;
  for (int64_t start = 0; start < size + 10; start++) {
    for (int64_t length = 0; length < max_frag_length; length++) {
      auto frag = base.fragment(start, length);
      for (int64_t i = 0; i < length; i++) {
        int64_t j = start + i;  // old index
        int64_t v = (j < 0 || j >= size) ? 0 : j;
        checkMemory(frag, i, v);
      }
      checkMemory(frag, length, 0);
      checkMemory(frag, length + 1, 0);
    }
  }
}

void checkEnclosingLoop(const Program& p, int64_t begin, int64_t end,
                        int64_t op_index) {
  if (begin >= 0 && end >= 0 &&
      (p.ops.at(begin).type != Operation::Type::LPB ||
       p.ops.at(end).type != Operation::Type::LPE)) {
    ProgramUtil::print(p, std::cout);
    Log::get().error("Unexpected enclosing loop test: " +
                         std::to_string(begin) + "-" + std::to_string(end),
                     true);
  }
  auto loop = ProgramUtil::getEnclosingLoop(p, op_index);
  if (loop.first != begin || loop.second != end) {
    Log::get().error(
        "Unexpected enclosing loop: " + std::to_string(loop.first) + "-" +
            std::to_string(loop.second) +
            "; expected: " + std::to_string(begin) + "-" + std::to_string(end),
        true);
  }
}

void Test::operationMetadata() {
  Log::get().info("Testing operation metadata");
  if (static_cast<size_t>(Operation::Type::__COUNT) !=
      Operation::Types.size()) {
    Log::get().error("Unexpected number of operation types", true);
  }
  std::set<std::string> names;
  for (auto type : Operation::Types) {
    auto& meta = Operation::Metadata::get(type);
    if (type != meta.type) {
      Log::get().error("Unexpected type: " + meta.name, true);
    }
    if (names.count(meta.name)) {
      Log::get().error("Duplicate name: " + meta.name, true);
    }
    names.insert(meta.name);
  }
}

void Test::programUtil() {
  Log::get().info("Testing program util");
  Parser parser;
  Program primes_const_loop, primes_var_loop;
  auto base_path = std::string("tests") + FILE_SEP + "programs" + FILE_SEP +
                   "util" + FILE_SEP;
  primes_const_loop = parser.parse(base_path + "primes_const_loop.asm");
  primes_var_loop = parser.parse(base_path + "primes_var_loop.asm");
  auto const_info = Constants::findConstantLoop(primes_const_loop);
  auto var_info = Constants::findConstantLoop(primes_var_loop);
  if (!const_info.has_constant_loop || const_info.index_lpb != 3 ||
      const_info.constant_value.asInt() != 7776) {
    Log::get().error("Expected contant loop in primes_const_loop.asm", true);
  }
  if (var_info.has_constant_loop) {
    Log::get().error("Unexpected contant loop in primes_var_loop.asm", true);
  }
  auto p = parser.parse(ProgramUtil::getProgramPath(1041));
  checkEnclosingLoop(p, 7, 13, 12);
  checkEnclosingLoop(p, 7, 13, 7);
  checkEnclosingLoop(p, 7, 13, 13);
  checkEnclosingLoop(p, 5, 17, 5);
  checkEnclosingLoop(p, 5, 17, 6);
  checkEnclosingLoop(p, 5, 17, 15);
  checkEnclosingLoop(p, 5, 17, 17);
  checkEnclosingLoop(p, -1, -1, 4);
  checkEnclosingLoop(p, -1, -1, 19);
  std::string com_in =
      "mov $1,26\n"
      "; Miner Profile: foobar\n"
      "add $1,$0\n";
  std::stringstream buf(com_in);
  p = parser.parse(buf);
  if (Comments::getCommentField(p, Comments::PREFIX_MINER_PROFILE) !=
      "foobar") {
    Log::get().error("Cannot extract miner profile from program comment", true);
  }
  Comments::removeCommentField(p, Comments::PREFIX_MINER_PROFILE);
  buf = std::stringstream();
  ProgramUtil::print(p, buf);
  std::string com_out =
      "mov $1,26\n"
      "add $1,$0\n";
  if (buf.str() != com_out) {
    Log::get().error("Unexpected program after removing comment: " + buf.str(),
                     true);
  }
  p = parser.parse(ProgramUtil::getProgramPath(45));
  auto h1 = ProgramUtil::hash(p);
  ProgramUtil::removeOps(p, Operation::Type::NOP);
  auto h2 = ProgramUtil::hash(p);
  if (h2 != h1) {
    Log::get().error("Unexpected program hash: " + std::to_string(h2), true);
  }
}

void validateIterated(const Program& p) {
  ProgramUtil::validate(p);
  if (ProgramUtil::numOps(p, Operand::Type::INDIRECT) > 0) {
    throw std::runtime_error("Iterator generated indirect memory access");
  }
  for (size_t i = 0; i < p.ops.size(); i++) {
    const auto& op = p.ops[i];
    if (op.type == Operation::Type::LPB &&
        (op.source.type != Operand::Type::CONSTANT ||
         !(Number::ZERO < op.source.value))) {
      throw std::runtime_error("Iterator generated wrong loop");
    }
    if (ProgramUtil::isWritingRegion(op.type) ||
        !Iterator::supportsOperationType(op.type)) {
      throw std::runtime_error("Unsupported operation type");
    }
  }
  for (size_t i = 1; i < p.ops.size(); i++) {
    if (p.ops[i - 1].type == Operation::Type::LPB &&
        p.ops[i].type == Operation::Type::LPE) {
      throw std::runtime_error("Iterator generated empty loop");
    }
  }
}

void Test::iterator(size_t tests) {
  const int64_t count = 100000;
  for (size_t test = 0; test < tests; test++) {
    if (test % 10 == 0) {
      Log::get().info("Testing iterator " + std::to_string(test));
    }

    // generate a random start program
    Generator::Config config;
    config.version = 1;
    config.loops = true;
    config.calls = false;
    config.indirect_access = false;

    config.length = std::max<int64_t>(test / 4, 2);
    config.max_constant = std::max<int64_t>(test / 4, 2);
    config.max_index = std::max<int64_t>(test / 4, 2);
    GeneratorV1 gen_v1(config, getManager().getStats());
    Program start, p, q;
    while (true) {
      try {
        start = gen_v1.generateProgram();
        validateIterated(start);
        break;
      } catch (const std::exception&) {
        // ignore
      }
    }
    // iterate and check
    Iterator it(start);
    for (int64_t i = 0; i < count; i++) {
      p = it.next();
      try {
        validateIterated(p);
        if (i > 0 && (p < q || !(q < p) || p == q)) {
          throw std::runtime_error("Iterator violates program order");
        }
      } catch (std::exception& e) {
        ProgramUtil::print(q, std::cerr);
        std::cerr << std::endl;
        ProgramUtil::print(p, std::cerr);
        Log::get().error(e.what(), true);
      }
      q = p;
    }
    if (it.getSkipped() > 0.01 * count) {
      Log::get().error("Too many skipped invalid programs: " +
                           std::to_string(it.getSkipped()),
                       true);
    }
  }
}

void Test::knownPrograms() {
  testSeq(5, Sequence(
                 {1, 2, 2, 3, 2, 4, 2, 4, 3, 4, 2, 6, 2, 4, 4, 5, 2, 6, 2, 6}));
  testSeq(30, Sequence({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1,
                        1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2}));
  testSeq(45, Sequence({0,  1,  1,   2,   3,   5,   8,   13,   21,   34,
                        55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181}));
  testSeq(79, Sequence({1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096,
                        8192, 16384, 32768, 65536}));
  testSeq(1489, Sequence({0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12,
                          -13, -14, -15, -16, -17}));
  testSeq(1497, Sequence({1,   1,   1,   3,     3,     1,    15,   15,  6,
                          1,   105, 105, 45,    10,    1,    945,  945, 420,
                          105, 15,  1,   10395, 10395, 4725, 1260, 210}));
  testSeq(2260, Sequence({1, 1, 2, 1, 2, 3, 1, 2, 3, 4,
                          1, 2, 3, 4, 5, 1, 2, 3, 4, 5}));
  testSeq(248765,
          Sequence({1,     1,     1,     1,      1,      2,      2,
                    2,     6,     12,    12,     12,     12,     12,
                    12,    24,    24,    144,    144,    720,    720,
                    720,   720,   1440,  1440,   1440,   4320,   60480,
                    60480, 60480, 60480, 120960, 120960, 241920, 1209600}));
}

void Test::fold() {
  auto tests =
      loadInOutTests(std::string("tests") + FILE_SEP + "fold" + FILE_SEP + "F");
  size_t i = 1;
  Parser parser;
  Evaluator evaluator(settings, false);
  std::map<int64_t, int64_t> cellMap;
  for (const auto& t : tests) {
    Log::get().info("Testing fold " + std::to_string(i));
    Operation seqOrPrg;
    for (const auto& op : t.second.ops) {
      if (op.type == Operation::Type::SEQ || op.type == Operation::Type::PRG) {
        seqOrPrg = op;
        break;
      }
    }
    if (seqOrPrg.type == Operation::Type::NOP) {
      Log::get().error("No seq or prg in output found", true);
    }
    int64_t subId = seqOrPrg.source.value.asInt();
    std::string path;
    if (seqOrPrg.type == Operation::Type::SEQ) {
      path = ProgramUtil::getProgramPath(subId);
    } else {
      path = ProgramUtil::getProgramPath(subId, "prg", "P");
    }
    auto sub = parser.parse(path);
    auto p = t.first;
    cellMap.clear();
    if (!Subprogram::fold(p, sub, subId, cellMap, settings.max_memory)) {
      Log::get().error("Folding not supported", true);
    }
    if (p != t.second) {
      ProgramUtil::print(p, std::cout);
      Log::get().error("Unexpected program", true);
    }
    Sequence expected, got;
    evaluator.eval(t.first, expected, 20);
    evaluator.eval(t.second, got, 20);
    if (expected != got) {
      Log::get().error("Unexpected sequence", true);
    }
    i++;
  }
}

void Test::unfold() {
  auto tests = loadInOutTests(std::string("tests") + FILE_SEP + "unfold" +
                              FILE_SEP + "U");
  size_t i = 1;
  Evaluator evaluator(settings, false);
  for (const auto& t : tests) {
    Log::get().info("Testing unfold " + std::to_string(i));
    auto p = t.first;
    if (!Subprogram::autoUnfold(p)) {
      Log::get().error("Unfolding not supported", true);
    }
    if (p != t.second) {
      ProgramUtil::print(p, std::cout);
      Log::get().error("Unexpected program", true);
    }
    Sequence expected, got;
    evaluator.eval(t.first, expected, 20);
    evaluator.eval(t.second, got, 20);
    if (expected != got) {
      Log::get().error("Unexpected sequence", true);
    }
    i++;
  }
}

void Test::incEval() {
  // manually written test cases
  size_t i = 1;
  auto dir = std::string("tests") + FILE_SEP + "inceval" + FILE_SEP;
  std::stringstream s;
  while (true) {
    s.str("");
    s << dir << "I" << std::setw(3) << std::setfill('0') << i << ".asm";
    auto path = s.str();
    if (!isFile(path)) {
      break;
    }
    checkIncEval(settings, 0, path, true);
    i++;
  }
  // OEIS sequence test cases
  std::vector<size_t> ids = {
      8,     45,    142,    165,    178,    204,   246,   253,   278,
      280,   407,   542,    933,    1075,   1091,  1117,  1304,  1353,
      1360,  1519,  1541,   1542,   1609,   2081,  3411,  7661,  7981,
      8581,  10362, 11218,  12866,  14979,  22564, 25774, 49349, 57552,
      79309, 80493, 122593, 130487, 247309, 302643};
  for (auto id : ids) {
    checkIncEval(settings, id, "", true);
  }
}

bool Test::checkIncEval(const Settings& settings, size_t id, std::string path,
                        bool mustSupportIncEval) {
  auto name = path;
  if (path.empty()) {
    name = ProgramUtil::idStr(id);
    path = ProgramUtil::getProgramPath(id);
  }
  Parser parser;
  Program p;
  try {
    p = parser.parse(path);
  } catch (const std::exception& e) {
    if (mustSupportIncEval) {
      std::rethrow_exception(std::current_exception());
    } else {
      Log::get().warn(std::string(e.what()));
      return false;
    }
  }
  const std::string msg = "incremental evaluator for " + name;
  Evaluator eval_reg(settings, false);
  Evaluator eval_inc(settings, true);
  if (!eval_inc.supportsIncEval(p)) {
    if (mustSupportIncEval) {
      Log::get().error("Error initializing " + msg, true);
    } else {
      return false;
    }
  }
  Log::get().info("Testing " + msg);
  // std::cout << ProgramUtil::operationToString(p.ops.front()) << std::endl;
  Sequence seq_reg, seq_inc;
  steps_t steps_reg, steps_inc;
  try {
    steps_reg = eval_reg.eval(p, seq_reg, 100, true);
    steps_inc = eval_inc.eval(p, seq_inc, 100, true);
  } catch (const std::exception&) {
    try {
      steps_reg = eval_reg.eval(p, seq_reg, 10, true);
      steps_inc = eval_inc.eval(p, seq_inc, 10, true);
    } catch (const std::exception& e) {
      if (mustSupportIncEval) {
        std::rethrow_exception(std::current_exception());
      } else {
        Log::get().warn(std::string(e.what()));
        return false;
      }
    }
  }
  if (seq_reg != seq_inc) {
    Log::get().info("Incremental eval result: " + seq_inc.to_string());
    Log::get().info("Regular eval result:     " + seq_reg.to_string());
    Log::get().error("Unexpected result of " + msg, true);
  }
  if (steps_reg.total != steps_inc.total) {
    Log::get().error("Unexpected steps of " + msg, true);
  }
  return true;
}

void Test::apiClient() {
  Log::get().info("Testing API client");
  ApiClient client;
  client.postProgram(std::string("tests") + FILE_SEP + "programs" + FILE_SEP +
                     "oeis" + FILE_SEP + "000" + FILE_SEP + "A000005.asm");
  auto program = client.getNextProgram();
  if (program.ops.empty()) {
    Log::get().error("Expected non-empty program from API server", true);
  }
}

void Test::checkpoint() {
  Log::get().info("Testing checkpoint");
  ProgressMonitor m(3600, "", "", 23495249857);
  uint32_t v = 123456;
  auto enc = m.encode(v);
  if (m.decode(enc) != v) {
    Log::get().error("Error in checkpoint cycle", true);
  }
  bool checksum_error_detected;
  try {
    m.decode(enc ^ 1);  // flipped bit => invalid checksum
    checksum_error_detected = false;
  } catch (const std::exception&) {
    checksum_error_detected = true;
  }
  if (!checksum_error_detected) {
    Log::get().error("Checksum error not detected", true);
  }
}

void Test::steps() {
  auto file = ProgramUtil::getProgramPath(12);
  Log::get().info("Testing steps for " + file);
  Parser parser;
  Interpreter interpreter(settings);
  auto p = parser.parse(file);
  Memory mem;
  mem.set(Program::INPUT_CELL, 26);
  auto steps = interpreter.run(p, mem);
  if (steps != 1) {
    Log::get().error("unexpected number of steps: " + std::to_string(steps),
                     true);
  }
}

void Test::blocks() {
  auto tests = loadInOutTests(std::string("tests") + FILE_SEP + "blocks" +
                              FILE_SEP + "B");
  size_t i = 1;
  Blocks::Collector collector;
  for (auto& t : tests) {
    Log::get().info("Testing blocks " + std::to_string(i));
    collector.add(t.first);
    auto blocks = collector.finalize();
    if (blocks.list != t.second) {
      ProgramUtil::print(blocks.list, std::cerr);
      Log::get().error("Unexpected blocks output", true);
    }
    Program list;
    for (size_t i = 0; i < blocks.offsets.size(); i++) {
      auto b = blocks.getBlock(i);
      Operation nop(Operation::Type::NOP);
      nop.comment = std::to_string(static_cast<int64_t>(blocks.rates.at(i)));
      list.ops.push_back(nop);
      list.ops.insert(list.ops.end(), b.ops.begin(), b.ops.end());
    }
    if (blocks.list != list) {
      ProgramUtil::print(blocks.list, std::cerr);
      Log::get().error("Unexpected blocks list", true);
    }
    i++;
  }
}

void checkSeq(const Sequence& s, size_t expected_size, size_t index,
              const Number& expected_value) {
  if (s.size() != expected_size) {
    Log::get().error("Unexpected number of terms: " + std::to_string(s.size()) +
                         " (expected " + std::to_string(expected_size) + ")",
                     true);
  }
  if (s[index] != expected_value) {
    Log::get().error("Unexpected terms: " + s.to_string(), true);
  }
}

void checkSeqAgainstTestBFile(int64_t seq_id, int64_t offset,
                              int64_t max_num_terms) {
  std::stringstream buf;
  OeisSequence t(seq_id);
  t.getTerms(max_num_terms).to_b_file(buf, offset);
  std::ifstream bfile(std::string("tests") + FILE_SEP + "sequence" + FILE_SEP +
                      ProgramUtil::idStr(seq_id, "b") + ".txt");
  std::string x, y;
  while (std::getline(bfile, x)) {
    if (!std::getline(buf, y)) {
      Log::get().error("Expected line in sequence: " + x, true);
    }
    if (x != y) {
      Log::get().error(
          "Unexpected line in sequence: " + y + " (expected " + x + ")", true);
    }
  }
  if (std::getline(buf, y)) {
    Log::get().error("Unexpected line in sequence: " + y, true);
  }
  bfile.close();
}

void Test::oeisList() {
  Log::get().info("Testing OEIS lists");
  std::map<size_t, int64_t> map;
  std::string path = OeisList::getListsHome() + "test.txt";
  OeisList::loadMap(path, map);
  if (!map.empty()) {
    Log::get().error("unexpected map content", true);
  }
  map[3] = 5;
  map[7] = 9;
  map[8] = 4;
  auto copy = map;
  OeisList::mergeMap("test.txt", map);
  if (!map.empty()) {
    Log::get().error("unexpected map content", true);
  }
  OeisList::loadMap(path, map);
  if (map != copy) {
    Log::get().error("unexpected map content", true);
  }
  std::map<size_t, int64_t> delta;
  delta[7] = 6;
  OeisList::mergeMap("test.txt", delta);
  OeisList::loadMap(path, map);
  copy[7] = 15;
  if (map != copy) {
    Log::get().error("unexpected map content", true);
  }
  std::remove(path.c_str());
}

void Test::oeisSeq() {
  Log::get().info("Testing OEIS sequences");
  OeisSequence s(6);
  // std::remove( s.getBFilePath().c_str() );
  checkSeq(s.getTerms(20), 20, 18, 8);  // this should fetch the b-file
  checkSeq(s.getTerms(250), 250, 235, 38);
  checkSeq(s.getTerms(2000), 2000, 1240, 100);
  checkSeq(s.getTerms(10000), 10000, 9840, 320);
  checkSeq(s.getTerms(100000), 10000, 9840,
           320);  // only 10000 terms available
  checkSeq(s.getTerms(10000), 10000, 9840,
           320);  // from here on, the b-file should not get re-loaded
  checkSeq(s.getTerms(2000), 2000, 1240, 100);
  checkSeq(s.getTerms(250), 250, 235, 38);
  checkSeq(s.getTerms(20), 20, 18, 8);
  checkSeqAgainstTestBFile(45, 0, 2000);
}

void Test::ackermann() {
  std::vector<std::vector<int64_t>> values = {{1, 2, 3, 4, 5},
                                              {2, 3, 4, 5, 6},
                                              {3, 5, 7, 9, 11},
                                              {5, 13, 29, 61, 125},
                                              {13, 65533}};
  testBinary("ack",
             std::string("tests") + FILE_SEP + "programs" + FILE_SEP +
                 "general" + FILE_SEP + "ackermann.asm",
             values);
}

void check_int(const std::string& s, int64_t expected, int64_t value) {
  if (value != expected) {
    Log::get().error("expected " + s + ": " + std::to_string(expected) +
                         ", got: " + std::to_string(value),
                     true);
  }
}

void check_str(const std::string& s, const std::string& expected,
               const std::string& value) {
  if (value != expected) {
    Log::get().error("expected " + s + ": " + expected + ", got: " + value,
                     true);
  }
}

void Test::config() {
  Log::get().info("Testing config");

  Settings settings;
  auto config = ConfigLoader::load(settings);
  check_int("overwrite", 1, config.overwrite_mode == OverwriteMode::NONE);

  std::string templates = std::string("tests") + FILE_SEP + "programs" +
                          FILE_SEP + "templates" + FILE_SEP;
  check_int("generators.size", 2, config.generators.size());
  check_int("generators[0].version", 1, config.generators[0].version);
  check_int("generators[0].length", 30, config.generators[0].length);
  check_int("generators[0].maxConstant", 3, config.generators[0].max_constant);
  check_int("generators[0].maxIndex", 4, config.generators[0].max_index);
  check_int("generators[0].loops", 0, config.generators[0].loops);
  check_int("generators[0].calls", 1, config.generators[0].calls);
  check_int("generators[0].indirectAccess", 0,
            config.generators[0].indirect_access);
  check_int("generators[0].template", 2, config.generators[0].templates.size());
  check_str("generators[0].template[0]", templates + "call.asm",
            config.generators[0].templates[0]);
  check_str("generators[0].template[1]", templates + "loop.asm",
            config.generators[0].templates[1]);
  check_int("generators[1].version", 1, config.generators[1].version);
  check_int("generators[1].length", 40, config.generators[1].length);
  check_int("generators[1].maxConstant", 4, config.generators[1].max_constant);
  check_int("generators[1].maxIndex", 5, config.generators[1].max_index);
  check_int("generators[1].loops", 1, config.generators[1].loops);
  check_int("generators[1].calls", 0, config.generators[1].calls);
  check_int("generators[1].indirectAccess", 1,
            config.generators[1].indirect_access);
  check_int("generators[1].template", 0, config.generators[1].templates.size());

  check_int("matchers.size", 2, config.matchers.size());
  check_str("matchers[0].type", "direct", config.matchers[0].type);
  check_int("matchers[0].backoff", 1, config.matchers[0].backoff);
  check_str("matchers[1].type", "linear1", config.matchers[1].type);
  check_int("matchers[1].backoff", 1, config.matchers[1].backoff);

  settings.miner_profile = "update";
  config = ConfigLoader::load(settings);
  check_int("overwrite", 1, config.overwrite_mode == OverwriteMode::ALL);

  check_int("generators.size", 2, config.generators.size());
  check_int("generators[0].version", 2, config.generators[0].version);
  check_int("generators[1].version", 3, config.generators[1].version);

  check_int("matchers.size", 2, config.matchers.size());
  check_str("matchers[0].type", "linear2", config.matchers[0].type);
  check_int("matchers[0].backoff", 0, config.matchers[0].backoff);
  check_str("matchers[1].type", "delta", config.matchers[1].type);
  check_int("matchers[1].backoff", 0, config.matchers[1].backoff);

  // test selecting miner configx using index instead of name
  settings.miner_profile = "0";
  config = ConfigLoader::load(settings);
  check_int("generators.size", 2, config.generators.size());
  check_int("generators[0].version", 1, config.generators[0].version);

  settings.miner_profile = "1";
  config = ConfigLoader::load(settings);
  check_int("generators.size", 2, config.generators.size());
  check_int("generators[0].version", 2, config.generators[0].version);
}

void Test::memUsage() {
  int64_t usage = getMemUsage() / (1024 * 1024);
  int64_t total = getTotalSystemMem() / (1024 * 1024);
  Log::get().info("Testing memory usage: " + std::to_string(usage) + "/" +
                  std::to_string(total) + " MiB");
  if (usage < 250 || usage > 1000) {
    Log::get().error("Unexpected memory usage", true);
  }
}

void Test::formula() {
  checkFormulas("formula.txt", FormulaType::FORMULA);
  checkFormulas("pari-function.txt", FormulaType::PARI_FUNCTION);
  checkFormulas("pari-vector.txt", FormulaType::PARI_VECTOR);
}

void Test::checkFormulas(const std::string& testFile, FormulaType type) {
  std::string path = std::string("tests") + FILE_SEP + std::string("formula") +
                     FILE_SEP + testFile;
  std::map<size_t, std::string> map;
  OeisList::loadMapWithComments(path, map);
  if (map.empty()) {
    Log::get().error("Unexpected map content", true);
  }
  Parser parser;
  FormulaGenerator generator;
  for (const auto& e : map) {
    auto id = e.first;
    Log::get().info("Testing formula for " + ProgramUtil::idStr(id) + ": " +
                    e.second);
    auto p = parser.parse(ProgramUtil::getProgramPath(id));
    Formula f;
    if (!generator.generate(p, id, f, true)) {
      Log::get().error("Cannot generate formula from program", true);
    }
    if (type == FormulaType::FORMULA) {
      if (f.toString() != e.second) {
        Log::get().error("Unexpected formula: " + f.toString(), true);
      }
    } else {
      PariFormula pari;
      if (!PariFormula::convert(f, type == FormulaType::PARI_VECTOR, pari)) {
        Log::get().error("Cannot convert formula to PARI/GP", true);
      }
      if (pari.toString() != e.second) {
        Log::get().error("Unexpected PARI/GP code: " + pari.toString(), true);
      }
    }
  }
}

void Test::range() {
  testRanges("range.txt", false);
  testRanges("range-finite.txt", true);
}

void Test::testRanges(const std::string& filename, bool finite) {
  std::string path = std::string("tests") + FILE_SEP + std::string("formula") +
                     FILE_SEP + filename;
  std::map<size_t, std::string> map;
  OeisList::loadMapWithComments(path, map);
  if (map.empty()) {
    Log::get().error("Unexpected map content", true);
  }
  Parser parser;
  for (const auto& e : map) {
    checkRanges(OeisSequence(e.first).id, finite, e.second);
  }
}

void Test::checkRanges(int64_t id, bool finite, const std::string& expected) {
  Parser parser;
  auto p = parser.parse(ProgramUtil::getProgramPath(id));
  auto offset = ProgramUtil::getOffset(p);
  Number inputUpperBound = finite ? offset + 9 : Number::INF;
  Log::get().info("Testing ranges for " + ProgramUtil::idStr(id) + ": " +
                  expected + " with upper bound " +
                  inputUpperBound.to_string());
  RangeGenerator generator;
  RangeMap ranges;
  if (!generator.generate(p, ranges, inputUpperBound)) {
    Log::get().error("Cannot generate range from program", true);
  }
  auto result = ranges.toString(Program::OUTPUT_CELL, "a(n)");
  if (result != expected) {
    Log::get().error("Unexpected ranges: " + result, true);
  }
}

void Test::stats() {
  Log::get().info("Testing stats loading and saving");

  // load stats
  Stats s, t;
  s = getManager().getStats();

  // sanity check for loaded stats
  if (s.num_constants.at(1) == 0) {
    Log::get().error("Error loading constants counts from stats", true);
  }
  if (s.num_ops_per_type.at(static_cast<size_t>(Operation::Type::MOV)) == 0) {
    Log::get().error("Error loading operation type counts from stats", true);
  }
  Operation op(Operation::Type::ADD, Operand(Operand::Type::DIRECT, 0),
               Operand(Operand::Type::CONSTANT, 1));
  if (s.num_operations.at(op) == 0) {
    Log::get().error("Error loading operation counts from stats", true);
  }
  if (s.num_operation_positions.size() < 100) {
    Log::get().error(
        "Unexpected number of operation position counts in stats: " +
            std::to_string(s.num_operation_positions.size()),
        true);
  }
  OpPos op_pos;
  op_pos.pos = 0;
  op_pos.len = 1;
  op_pos.op.type = Operation::Type::MOV;
  op_pos.op.target = Operand(Operand::Type::DIRECT, 0);
  op_pos.op.source = Operand(Operand::Type::CONSTANT, 1);
  if (s.num_operation_positions.at(op_pos) == 0) {
    Log::get().error("Error loading operation position counts from stats",
                     true);
  }
  if (!s.all_program_ids.at(5)) {
    Log::get().error("Error loading program summary from stats", true);
  }
  if (!s.program_lengths.at(7)) {
    Log::get().error("Error loading program lengths from stats", true);
  }
  if (!s.call_graph.count(168380)) {
    Log::get().error("Unexpected call graph for A168380", true);
  }
  auto l = s.getTransitiveLength(168380);
  if (l != 13) {
    Log::get().error(
        "Unexpected transitive length of A168380: " + std::to_string(l), true);
  }

  // save & reload stats
  std::string dir = getTmpDir() + "stats2";
  ensureTrailingFileSep(dir);
  ensureDir(dir);
  s.save(dir);
  t.load(dir);

  // compare loaded to original
  for (auto& e : s.num_constants) {
    auto m = e.second;
    auto n = t.num_constants.at(e.first);
    if (m != n) {
      Log::get().error("Unexpected number of constants count: " +
                           std::to_string(m) + "!=" + std::to_string(n),
                       true);
    }
  }
  for (size_t i = 0; i < s.num_ops_per_type.size(); i++) {
    auto m = s.num_ops_per_type.at(i);
    auto n = t.num_ops_per_type.at(i);
    if (m != n) {
      Log::get().error("Unexpected number of operation type count: " +
                           std::to_string(m) + "!=" + std::to_string(n),
                       true);
    }
  }
  for (auto it : s.num_operations) {
    if (it.second != t.num_operations.at(it.first)) {
      Log::get().error("Unexpected number of operations count", true);
    }
  }
  for (auto it : s.num_operation_positions) {
    if (it.second != t.num_operation_positions.at(it.first)) {
      Log::get().error("Unexpected number of operation position count", true);
    }
  }
  if (s.all_program_ids.size() != t.all_program_ids.size()) {
    Log::get().error("Unexpected number of found programs: " +
                         std::to_string(s.all_program_ids.size()) +
                         "!=" + std::to_string(t.all_program_ids.size()),
                     true);
  }
  for (size_t i = 0; i < s.all_program_ids.size(); i++) {
    auto a = s.all_program_ids.at(i);
    auto b = t.all_program_ids.at(i);
    if (a != b) {
      Log::get().error("Unexpected found programs for: " + std::to_string(i),
                       true);
    }
  }
}

void Test::optimizer() {
  Settings settings;
  Interpreter interpreter(settings);
  Optimizer optimizer(settings);
  auto tests = loadInOutTests(std::string("tests") + FILE_SEP + "optimizer" +
                              FILE_SEP + "E");
  size_t i = 1;
  for (auto& t : tests) {
    Log::get().info("Testing optimizer " + std::to_string(i));
    optimizer.optimize(t.first);
    if (t.first != t.second) {
      ProgramUtil::print(t.first, std::cerr);
      Log::get().error(
          "Unexpected optimized output for test " + std::to_string(i), true);
    }
    i++;
  }
}

void Test::minimizer(size_t tests) {
  Evaluator evaluator(settings);
  Minimizer minimizer(settings);
  MultiGenerator multi_generator(settings, getManager().getStats(), false);
  Sequence s1, s2, s3;
  Program program, minimized;
  const int64_t num_tests = tests;
  for (int64_t i = 0; i < num_tests; i++) {
    if (i % (num_tests / 10) == 0) {
      Log::get().info("Testing minimizer " + std::to_string(i));
    }
    program = multi_generator.generateProgram();
    try {
      evaluator.eval(program, s1, OeisSequence::DEFAULT_SEQ_LENGTH);
      if (s1.size() != OeisSequence::DEFAULT_SEQ_LENGTH) {
        i--;
        continue;
      }
    } catch (const std::exception& e) {
      i--;
      continue;
    }
    minimized = program;
    minimizer.optimizeAndMinimize(minimized, s1.size());
    evaluator.eval(minimized, s2, s1.size());
    if (s1.size() != s2.size() || (s1 != s2)) {
      std::cout << "before: " << s1 << std::endl;
      ProgramUtil::print(program, std::cout);
      std::cout << "after:  " << s2 << std::endl;
      ProgramUtil::print(minimized, std::cout);
      Log::get().error(
          "Program evaluated to different sequence after minimization", true);
    }
  }
}

bool checkRange(const Sequence& seq, const Program& program, bool finiteInput) {
  auto offset = ProgramUtil::getOffset(program);
  Number inputUpperBound = finiteInput ? offset + seq.size() - 1 : Number::INF;
  RangeGenerator generator;
  RangeMap ranges;
  try {
    if (!generator.generate(program, ranges, inputUpperBound)) {
      return false;
    }
  } catch (const std::exception& e) {
    ProgramUtil::print(program, std::cout);
    Log::get().error(
        "Error during range generation for program " + std::string(e.what()),
        true);
  }
  auto it = ranges.find(Program::OUTPUT_CELL);
  if (it == ranges.end()) {
    return false;
  }
  auto result = ranges.toString(Program::OUTPUT_CELL, "a(n)");
  auto& range = it->second;
  // Log::get().info("Checking " + std::to_string(seq.size()) + " terms" + ": "
  // +
  //                result);
  auto index = range.check(seq);
  if (index != -1) {
    ProgramUtil::print(program, std::cout);
    Log::get().error("Range check failed for a(" +
                         std::to_string(index + offset) +
                         ") = " + seq[index].to_string() +
                         " with upper bound " + inputUpperBound.to_string(),
                     true);
  }
  return true;
}

void Test::randomRange(size_t tests) {
  Evaluator evaluator(settings);
  MultiGenerator multi_generator(settings, getManager().getStats(), false);
  Sequence seq;
  Program program;
  const int64_t num_tests = tests;
  for (int64_t i = 0; i < num_tests; i++) {
    if (i % (num_tests / 10) == 0) {
      Log::get().info("Testing random range " + std::to_string(i));
    }
    program = multi_generator.generateProgram();
    ProgramUtil::setOffset(program, i % 3);
    try {
      evaluator.eval(program, seq, OeisSequence::DEFAULT_SEQ_LENGTH);
      if (seq.size() != OeisSequence::DEFAULT_SEQ_LENGTH) {
        i--;  // try another program
        continue;
      }
    } catch (const std::exception& e) {
      i--;  // try another program
      continue;
    }
    if (!checkRange(seq, program, true) || !checkRange(seq, program, false)) {
      i--;  // try another program
      continue;
    }
  }
}

void Test::miner() {
  Log::get().info("Testing miner");
  Git::git("", "--version");
  getManager().load();
  getManager().getFinder();
  MultiGenerator multi_generator(settings, getManager().getStats(), true);
}

void Test::linearMatcher() {
  LinearMatcher matcher(false);
  testMatcherSet(matcher, {27, 5843, 8585, 16789});
  testMatcherSet(matcher, {290, 1105, 117950});
}

void Test::deltaMatcher() {
  DeltaMatcher matcher(false);
  testMatcherSet(matcher, {7, 12, 27});
  testMatcherSet(matcher, {108, 14137});
  testMatcherSet(matcher, {4273, 290, 330});
  testMatcherSet(matcher, {168380, 193356});
  testMatcherSet(matcher, {243980, 244050});
}

void Test::digitMatcher() {
  DigitMatcher binary("binary", 2, false);
  testMatcherPair(binary, 1477, 35);
  testMatcherPair(binary, 16789, 35);
  DigitMatcher decimal("decimal", 10, false);
  testMatcherPair(decimal, 8593, 10879);
  // testMatcherPair( decimal, 11557, 7 );
}

void Test::testBinary(const std::string& func, const std::string& file,
                      const std::vector<std::vector<int64_t>>& values) {
  Log::get().info("Testing " + file);
  Parser parser;
  Interpreter interpreter(settings);
  auto program = parser.parse(file);
  for (size_t i = 0; i < values.size(); i++) {
    for (size_t j = 0; j < values[i].size(); j++) {
      Memory mem;
      mem.set(0, i);
      mem.set(1, j);
      interpreter.run(program, mem);
      if (mem.get(2) != values[i][j]) {
        Log::get().error("unexpected result: " + mem.get(2).to_string(), true);
      }
    }
  }
}

std::vector<std::pair<Program, Program>> Test::loadInOutTests(
    const std::string& prefix) {
  Parser parser;
  size_t i = 1;
  std::vector<std::pair<Program, Program>> result;
  while (true) {
    std::stringstream s;
    s << prefix << std::setw(3) << std::setfill('0') << i << ".asm";
    std::ifstream file(s.str());
    if (!file.good()) {
      break;
    }
    auto p = parser.parse(file);
    int in = -1, out = -1;
    for (size_t i = 0; i < p.ops.size(); i++) {
      if (p.ops[i].type == Operation::Type::NOP) {
        if (in == -1 && p.ops[i].comment == "in") {
          in = i;
        }
        if (out == -1 && p.ops[i].comment == "out") {
          out = i;
        }
      }
    }
    if (in < 0 || out < 0 || in >= out) {
      Log::get().error("Error parsing test", true);
    }
    std::pair<Program, Program> inout;
    inout.first.ops.insert(inout.first.ops.begin(), p.ops.begin() + in + 1,
                           p.ops.begin() + out);
    inout.second.ops.insert(inout.second.ops.begin(), p.ops.begin() + out + 1,
                            p.ops.end());
    result.emplace_back(std::move(inout));
    i++;
  }
  return result;
}

void Test::testSeq(size_t id, const Sequence& expected) {
  auto file = ProgramUtil::getProgramPath(id);
  Log::get().info("Testing " + file);
  Parser parser;
  Settings settings;  // special settings
  settings.num_terms = expected.size();
  Evaluator evaluator(settings);
  auto p = parser.parse(file);
  Sequence result;
  evaluator.eval(p, result);
  if (result != expected) {
    Log::get().error("unexpected result: " + result.to_string(), true);
  }
}

void Test::testMatcherSet(Matcher& matcher, const std::vector<size_t>& ids) {
  for (auto id1 : ids) {
    for (auto id2 : ids) {
      testMatcherPair(matcher, id1, id2);
    }
  }
}

void eval(const Program& p, Evaluator& evaluator, Sequence& s) {
  try {
    evaluator.eval(p, s);
  } catch (const std::exception& e) {
    ProgramUtil::print(p, std::cerr);
    Log::get().error("Error evaluating program: " + std::string(e.what()),
                     true);
  }
}

void Test::testMatcherPair(Matcher& matcher, size_t id1, size_t id2) {
  Log::get().info("Testing " + matcher.getName() + " matcher for " +
                  ProgramUtil::idStr(id1) + " -> " + ProgramUtil::idStr(id2));
  Parser parser;
  Evaluator evaluator(settings);
  auto p1 = parser.parse(ProgramUtil::getProgramPath(id1));
  auto p2 = parser.parse(ProgramUtil::getProgramPath(id2));
  ProgramUtil::removeOps(p1, Operation::Type::NOP);
  ProgramUtil::removeOps(p2, Operation::Type::NOP);
  Sequence s1, s2, s3;
  eval(p1, evaluator, s1);
  eval(p2, evaluator, s2);
  matcher.insert(s2, id2);
  Matcher::seq_programs_t result;
  matcher.match(p1, s1, result);
  matcher.remove(s2, id2);
  if (result.size() != 1) {
    Log::get().error(matcher.getName() + " matcher unable to match sequence",
                     true);
  }
  if (result[0].first != id2) {
    Log::get().error(
        matcher.getName() + " matcher returned unexpected sequence ID", true);
  }
  eval(result[0].second, evaluator, s3);
  if (s2.size() != s3.size() || s2 != s3) {
    ProgramUtil::print(result[0].second, std::cout);
    Log::get().error(matcher.getName() +
                         " matcher generated wrong program for " +
                         ProgramUtil::idStr(id2),
                     true);
  }
}
