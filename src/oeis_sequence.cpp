#include "oeis_sequence.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

#include "big_number.hpp"
#include "file.hpp"
#include "parser.hpp"
#include "setup.hpp"
#include "util.hpp"

const size_t OeisSequence::DEFAULT_SEQ_LENGTH = 100;

const size_t OeisSequence::EXTENDED_SEQ_LENGTH = 2000;

bool OeisSequence::isTooBig(const Number& n) {
  if (n == Number::INF) {
    return true;
  }
  if (USE_BIG_NUMBER) {
    return n.getNumUsedWords() > (BigNumber::NUM_WORDS / 2);
  } else {
    static const int64_t NUM_INF = std::numeric_limits<int64_t>::max();
    return (n.value > (NUM_INF / 1000)) || (n.value < (NUM_INF / -1000));
  }
}

OeisSequence::OeisSequence(size_t id) : id(id), num_bfile_terms(0) {}

OeisSequence::OeisSequence(std::string id_str) : num_bfile_terms(0) {
  if (id_str.empty() || id_str[0] != 'A') {
    throw std::invalid_argument(id_str);
  }
  id_str = id_str.substr(1);
  for (char c : id_str) {
    if (!std::isdigit(c)) {
      throw std::invalid_argument("A" + id_str);
    }
  }
  id = std::stoll(id_str);
}

OeisSequence::OeisSequence(size_t id, const std::string& name,
                           const Sequence& full)
    : id(id), name(name), terms(full), num_bfile_terms(0) {}

std::ostream& operator<<(std::ostream& out, const OeisSequence& s) {
  out << s.id_str() << ": " << s.name;
  return out;
}

std::string OeisSequence::to_string() const {
  std::stringstream ss;
  ss << (*this);
  return ss.str();
}

std::string OeisSequence::id_str(const std::string& prefix) const {
  std::stringstream s;
  s << prefix << std::setw(6) << std::setfill('0') << id;
  return s.str();
}

std::string OeisSequence::dir_str() const {
  std::stringstream s;
  s << std::setw(3) << std::setfill('0') << (id / 1000);
  return s.str();
}

std::string OeisSequence::url_str() const {
  return "https://oeis.org/" + id_str();
}

std::string OeisSequence::getProgramPath(bool local) const {
  if (local) {
    return Setup::getProgramsHome() + "local/" + id_str() + ".asm";
  } else {
    return Setup::getProgramsHome() + "oeis/" + dir_str() + "/" + id_str() +
           ".asm";
  }
}

std::string OeisSequence::getBFilePath() const {
  return Setup::getOeisHome() + "b/" + dir_str() + "/" + id_str("b") + ".txt";
}

Sequence loadBFile(size_t id, const Sequence& seq_full) {
  const OeisSequence oeis_seq(id);
  Sequence result;

  // try to read b-file
  std::ifstream big_file(oeis_seq.getBFilePath());
  std::string buf;
  if (big_file.good()) {
    std::string l;
    int64_t expected_index = -1, index = 0;
    while (std::getline(big_file, l)) {
      l.erase(l.begin(), std::find_if(l.begin(), l.end(), [](int ch) {
                return !std::isspace(ch);
              }));
      if (l.empty() || l[0] == '#') {
        continue;
      }
      // TODO: avoid extra buffer
      std::stringstream ss(l);
      ss >> index;
      if (expected_index == -1) {
        expected_index = index;
      }
      if (index != expected_index) {
        Log::get().warn("Unexpected index " + std::to_string(index) +
                        " in b-file " + oeis_seq.getBFilePath());
        result.clear();
        return result;
      }
      ss >> std::ws;
      Number::readIntString(ss, buf);
      Number value(buf);
      if (!ss || OeisSequence::isTooBig(value)) {
        break;
      }
      result.push_back(value);
      ++expected_index;
    }
    if (Log::get().level == Log::Level::DEBUG) {
      Log::get().debug("Read b-file for " + oeis_seq.id_str() + " with " +
                       std::to_string(result.size()) + " terms");
    }
  }

  // not found?
  if (result.empty()) {
    if (Log::get().level == Log::Level::DEBUG) {
      Log::get().debug("b-file not found or empty: " + oeis_seq.id_str());
    }
    return result;
  }

  // align sequences on common prefix (will verify correctness below again!)
  result.align(seq_full, 5);

  // check length
  std::string error_state;

  if (result.size() < seq_full.size()) {
    // big should never be shorter (there can be parser issues causing this)
    result = seq_full;
  }

  if (result.empty()) {
    error_state = "empty";
  } else {
    // check that the sequences agree on prefix
    auto seq_test = result.subsequence(0, seq_full.size());
    if (seq_test != seq_full) {
      Log::get().warn("Unexpected terms in b-file or program for " +
                      oeis_seq.id_str());
      Log::get().warn("- expected: " + seq_full.to_string());
      Log::get().warn("- found:    " + seq_test.to_string());
      error_state = "invalid";
    }
  }

  // remove b-files if they are issues (we use a heuristic to avoid massive
  // amount of downloads at the same time)
  if (!error_state.empty()) {
    // TODO: also re-fetch old files, see getFileAgeInDays(
    // oeis_seq.getBFilePath() )
    Log::get().warn("Removing " + error_state + " b-file " +
                    oeis_seq.getBFilePath());
    std::remove(oeis_seq.getBFilePath().c_str());
    return result;
  }

  if (Log::get().level == Log::Level::DEBUG) {
    Log::get().debug("Loaded long version of sequence " + oeis_seq.id_str() +
                     " with " + std::to_string(result.size()) + " terms");
  }
  return result;
}

Sequence OeisSequence::getTerms(int64_t max_num_terms) const {
  // determine real number of terms
  size_t real_max_terms =
      (max_num_terms >= 0) ? max_num_terms : EXTENDED_SEQ_LENGTH;

  // already have enough terms?
  if (real_max_terms <= terms.size()) {
    return terms.subsequence(0, real_max_terms);
  }

  if (id == 0) {
    Log::get().error("Invalid OEIS sequence ID", true);
  }

  // try to (re-)load b-file if not loaded yet or if there are more terms
  // available
  if (num_bfile_terms == 0 || num_bfile_terms > terms.size()) {
    const auto path = getBFilePath();
    auto big = loadBFile(id, terms);
    if (big.empty()) {
      // fetch b-file
      std::ifstream big_file(path);
      if (!big_file.good() ||
          big_file.peek() == std::ifstream::traits_type::eof()) {
        ensureDir(path);
        std::remove(path.c_str());
        Http::get(url_str() + "/" + id_str("b") + ".txt", path);
        big = loadBFile(id, terms);
      }
    }
    if (big.empty()) {
      Log::get().error("Error loading b-file " + path, true);
    }
    num_bfile_terms = big.size();

    // shrink big sequence to maximum number of terms
    if (big.size() > real_max_terms) {
      big = big.subsequence(0, real_max_terms);
    }

    // replace terms
    terms = big;
  }

  return terms;
}
