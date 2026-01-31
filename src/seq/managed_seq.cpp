#include "seq/managed_seq.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "math/big_number.hpp"
#include "mine/api_client.hpp"
#include "seq/seq_util.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"
#include "sys/web_client.hpp"

ManagedSequence::ManagedSequence(UID id)
    : id(id), offset(0), num_bfile_terms(0) {}

ManagedSequence::ManagedSequence(UID id, const std::string& name,
                                 const Sequence& full)
    : id(id), name(name), offset(0), terms(full), num_bfile_terms(0) {}

std::ostream& operator<<(std::ostream& out, const ManagedSequence& s) {
  out << s.id.string() << ": " << s.name;
  return out;
}

std::string ManagedSequence::string() const {
  std::stringstream ss;
  ss << (*this);
  return ss.str();
}

std::string ManagedSequence::getBFilePath() const {
  const std::string seqs_home = SequenceUtil::getSeqsFolder(id.domain());
  const std::string dir = ProgramUtil::dirStr(id);
  const std::string bfile = "b" + id.string().substr(1) + ".txt";
  return seqs_home + "b" + FILE_SEP + dir + FILE_SEP + bfile;
}

void ManagedSequence::removeInvalidBFile(const std::string& error) const {
  if (id.domain() == 'A') {  // only remove OEIS b-files
    auto path = getBFilePath();
    if (isFile(path)) {
      Log::get().warn("Removing " + error + " b-file " + path);
      std::remove(path.c_str());
      // report broken b-file to API server
      ApiClient::getDefaultInstance().reportBrokenBFile(id);
    }
  }
}

Sequence ManagedSequence::loadBFile() const {
  Sequence result;

  // try to read b-file
  try {
    std::ifstream big_file(getBFilePath());
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
          Log::get().error("Unexpected index " + std::to_string(index) +
                               " in b-file " + getBFilePath(),
                           false);
          removeInvalidBFile();
          result.clear();
          return result;
        }
        ss >> std::ws;
        Number::readIntString(ss, buf);
        Number value(buf);
        if (!ss || SequenceUtil::isTooBig(value)) {
          break;
        }
        result.push_back(value);
        ++expected_index;
      }
      if (Log::get().level == Log::Level::DEBUG) {
        Log::get().debug("Read b-file for " + id.string() + " with " +
                         std::to_string(result.size()) + " terms");
      }
    }
  } catch (const std::exception& e) {
    Log::get().error("Error reading b-file " + getBFilePath() + ": " + e.what(),
                     false);
    removeInvalidBFile();
    result.clear();
    return result;
  }

  // not found or empty?
  if (result.empty()) {
    Log::get().debug("b-file not found or empty: " + getBFilePath());
    removeInvalidBFile("empty");
    return result;
  }

  // align sequences on common prefix (will verify correctness below again!)
  result.align(terms, 5);

  // check length
  std::string error_state;

  if (result.size() < terms.size()) {
    // big should never be shorter (there can be parser issues causing this)
    result = terms;
  }

  if (result.empty()) {
    error_state = "empty";
  } else {
    // check that the sequences agree on prefix
    auto test = result.subsequence(0, terms.size());
    if (test != terms) {
      Log::get().warn("Unexpected terms in b-file or program for " +
                      id.string());
      Log::get().warn("- expected: " + terms.to_string());
      Log::get().warn("- found:    " + test.to_string());
      error_state = "invalid";
    }
  }

  // remove b-files if they are issues (we use a heuristic to avoid massive
  // amount of downloads at the same time)
  if (!error_state.empty()) {
    // TODO: also re-fetch old files, see getFileAgeInDays(
    // oeis_seq.getBFilePath() )
    removeInvalidBFile(error_state);
    result.clear();
    return result;
  }

  if (Log::get().level == Log::Level::DEBUG) {
    Log::get().debug("Loaded long version of sequence " + id.string() +
                     " with " + std::to_string(result.size()) + " terms");
  }
  return result;
}

Sequence ManagedSequence::getTerms(int64_t max_num_terms) const {
  // determine real number of terms
  size_t real_max_terms =
      (max_num_terms >= 0) ? max_num_terms : SequenceUtil::EXTENDED_SEQ_LENGTH;

  // already have enough terms?
  if (real_max_terms <= terms.size()) {
    return terms.subsequence(0, real_max_terms);
  }

  if (id.number() == 0) {
    Log::get().error("Invalid sequence ID: " + id.string(), true);
  }

  // try to (re-)load b-file if not loaded yet or if there are more terms
  // available
  if (num_bfile_terms == 0 || num_bfile_terms > terms.size()) {
    const auto path = getBFilePath();
    auto big = loadBFile();
    if (big.empty() && id.domain() == 'A') {
      // fetch b-file
      std::ifstream big_file(path);
      if (!big_file.good() ||
          big_file.peek() == std::ifstream::traits_type::eof()) {
        ensureDir(path);
        std::remove(path.c_str());
        std::string bfile = "b" + id.string().substr(1) + ".txt";
        ApiClient::getDefaultInstance().getOeisFile(bfile, path);
        big = loadBFile();
      }
    }
    if (big.empty()) {
      if (id.domain() == 'A') {
        Log::get().error("Error loading b-file " + path, true);
      } else {
        Log::get().warn("Missing b-file for " + id.string());
        big = terms;  // use what we have
      }
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
