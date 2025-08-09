#include "lang/subprogram.hpp"

bool updateOperand(const Operand &src, const Operand &trg,
                   std::map<int64_t, int64_t> &cell_map) {
  if (src.type != trg.type) {
    return false;
  }
  switch (src.type) {
    case Operand::Type::CONSTANT: {
      if (src.value != trg.value) {
        return false;
      }
      break;
    }
    case Operand::Type::DIRECT: {
      auto s = src.value.asInt();
      auto t = trg.value.asInt();
      auto it = cell_map.find(s);
      if (it == cell_map.end()) {
        cell_map[s] = t;
      } else if (it->second != t) {
        return false;
      }
      break;
    }
    case Operand::Type::INDIRECT: {
      return false;
    }
  }
  return true;
}

int64_t Subprogram::search(const Program &main, const Program &sub,
                           std::map<int64_t, int64_t> &cell_map) {
  // find a match
  size_t main_pos = 0, sub_pos = 0;
  while (sub_pos < sub.ops.size() && main_pos < main.ops.size()) {
    bool reset = false;
    if (sub.ops[sub_pos].type != main.ops[main_pos].type) {
      reset = true;
    } else if (!updateOperand(sub.ops[sub_pos].target,
                              main.ops[main_pos].target, cell_map)) {
      reset = true;
    } else if (!updateOperand(sub.ops[sub_pos].source,
                              main.ops[main_pos].source, cell_map)) {
      reset = true;
    }
    if (reset) {
      main_pos = main_pos - sub_pos + 1;
      sub_pos = 0;
      cell_map.clear();
    } else {
      main_pos++;
      sub_pos++;
    }
  }
  // ensure we matched full subprogram
  if (sub_pos < sub.ops.size()) {
    return -1;
  }
  return main_pos - sub_pos;
}

size_t Subprogram::replaceAllExact(Program &main, const Program &search,
                                   const Program &replace) {
  if (main.ops.empty() || search.ops.empty() ||
      search.ops.size() > main.ops.size()) {
    return 0;
  }
  size_t count = 0;
  const size_t max_start = main.ops.size() - search.ops.size();
  for (size_t i = 0; i < max_start; i++) {
    bool matches = true;
    for (size_t j = 0; j < search.ops.size(); j++) {
      if (main.ops[i + j] != search.ops[j]) {
        matches = false;
        break;
      }
    }
    if (matches) {
      main.ops.erase(main.ops.begin() + i,
                     main.ops.begin() + i + search.ops.size());
      main.ops.insert(main.ops.begin() + i, replace.ops.begin(),
                      replace.ops.end());
      i += replace.ops.size() - 1;
      count++;
    }
  }
  return count;
}
