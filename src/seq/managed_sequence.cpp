#include "seq/managed_sequence.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>

#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "math/big_number.hpp"
#include "mine/api_client.hpp"
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

std::string ManagedSequence::to_string() const {
  std::stringstream ss;
  ss << (*this);
  return ss.str();
}

std::string ManagedSequence::getBFilePath() const {
  std::string bfile = "b" + id.string().substr(1) + ".txt";
  return Setup::getOeisHome() + "b" + FILE_SEP + ProgramUtil::dirStr(id) +
         FILE_SEP + bfile;
}

void removeInvalidBFile(const ManagedSequence& oeis_seq,
                        const std::string& error = "invalid") {
  auto path = oeis_seq.getBFilePath();
  if (isFile(path)) {
    Log::get().warn("Removing " + error + " b-file " + path);
    std::remove(path.c_str());
  }
}

Sequence loadBFile(UID id, const Sequence& seq_full) {
  const ManagedSequence oeis_seq(id);
  Sequence result;

  // try to read b-file
  try {
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
          Log::get().error("Unexpected index " + std::to_string(index) +
                               " in b-file " + oeis_seq.getBFilePath(),
                           false);
          removeInvalidBFile(oeis_seq);
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
    Log::get().error(
        "Error reading b-file " + oeis_seq.getBFilePath() + ": " + e.what(),
        false);
    removeInvalidBFile(oeis_seq);
    result.clear();
    return result;
  }

  // not found or empty?
  if (result.empty()) {
    Log::get().debug("b-file not found or empty: " + oeis_seq.getBFilePath());
    removeInvalidBFile(oeis_seq, "empty");
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
                      id.string());
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
    removeInvalidBFile(oeis_seq, error_state);
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
        std::string bfile = "b" + id.string().substr(1) + ".txt";
        ApiClient::getDefaultInstance().getOeisFile(bfile, path);
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

bool OeisSeqList::exists(UID id) const {
  auto it = find(id.domain());
  if (it == end()) {
    return false;
  }
  const auto& seqs = it->second;
  auto index = id.number();
  if (index < 0 || index >= static_cast<int64_t>(seqs.size())) {
    return false;
  }
  return seqs[index].id == id;
}

const ManagedSequence& OeisSeqList::get(UID uid) const {
  return at(uid.domain()).at(uid.number());
}

ManagedSequence& OeisSeqList::get(UID uid) {
  return (*this)[uid.domain()][uid.number()];
}

void OeisSeqList::add(ManagedSequence&& seq) {
  auto& seqs = (*this)[seq.id.domain()];
  auto index = seq.id.number();
  if (index >= static_cast<int64_t>(seqs.size())) {
    seqs.resize(static_cast<int64_t>(1.5 * index) + 1);
  }
  seqs[index] = std::move(seq);
}
