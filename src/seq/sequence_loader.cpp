#include "seq/sequence_loader.hpp"

#include <chrono>
#include <fstream>
#include <sstream>

#include "oeis/oeis_list.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/util.hpp"

void throwParseError(const std::string &line) {
  Log::get().error("Error parse line: " + line, true);
}

SequenceLoader::SequenceLoader(SequenceIndex &index, size_t min_num_terms)
    : index(index), min_num_terms(min_num_terms), num_loaded(0), num_total(0) {}

void SequenceLoader::load(std::string folder, char domain) {
  if (!checkFolderDomain(folder, domain)) {
    return;  // already loaded
  }
  Log::get().debug("Loading sequences from folder " + folder +
                   " with domain '" + std::string(1, domain) + "'");
  auto start_time = std::chrono::steady_clock::now();

  loadData(folder, domain);
  loadNames(folder, domain);
  loadOffsets(folder, domain);

  // print summary
  auto cur_time = std::chrono::steady_clock::now();
  double duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        cur_time - start_time)
                        .count() /
                    1000.0;
  std::stringstream buf;
  buf.setf(std::ios::fixed);
  buf.precision(2);
  buf << duration;
  Log::get().info("Loaded " + std::to_string(num_loaded) + "/" +
                  std::to_string(num_total) + " \"" + std::string(1, domain) +
                  "\"-sequences in " + buf.str() + "s");
}

void SequenceLoader::loadData(const std::string &folder, char domain) {
  const std::string path = folder + "stripped";
  Log::get().debug("Loading sequence data from \"" + path + "\"");
  std::ifstream stripped(path);
  if (!stripped.good()) {
    Log::get().error("Sequence data not found: " + path, true);
  }
  std::string line;
  std::string buf;
  size_t pos;
  size_t id;
  Sequence seq_full, seq_big;
  while (std::getline(stripped, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    if (line[0] != domain) {
      throwParseError(line);
    }
    num_total++;
    id = 0;
    for (pos = 1; pos < line.length() && line[pos] >= '0' && line[pos] <= '9';
         ++pos) {
      id = (10 * id) + (line[pos] - '0');
    }
    if (pos >= line.length() || line[pos] != ' ' || id == 0) {
      throwParseError(line);
    }
    ++pos;
    if (pos >= line.length() || line[pos] != ',') {
      throwParseError(line);
    }
    ++pos;
    buf.clear();
    seq_full.clear();
    while (pos < line.length()) {
      if (line[pos] == ',') {
        Number num(buf);
        if (SequenceUtil::isTooBig(num)) {
          break;
        }
        seq_full.push_back(num);
        buf.clear();
      } else if ((line[pos] >= '0' && line[pos] <= '9') || line[pos] == '-') {
        buf += line[pos];
      } else {
        throwParseError(line);
      }
      ++pos;
    }

    // check minimum number of terms
    if (seq_full.size() < min_num_terms) {
      continue;
    }

    // add sequence to index
    index.add(ManagedSequence(UID(domain, id), "", seq_full));
    num_loaded++;
  }
}

void SequenceLoader::loadNames(const std::string &folder, char domain) {
  const std::string path = folder + "names";
  Log::get().debug("Loading sequence names from \"" + path + "\"");
  std::ifstream names(path);
  if (!names.good()) {
    Log::get().error("Sequence names not found: " + path, true);
  }
  std::string line;
  size_t pos;
  size_t id;
  while (std::getline(names, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    if (line[0] != domain) {
      throwParseError(line);
    }
    id = 0;
    for (pos = 1; pos < line.length() && line[pos] >= '0' && line[pos] <= '9';
         ++pos) {
      id = (10 * id) + (line[pos] - '0');
    }
    if (pos >= line.length() || line[pos] != ' ' || id == 0) {
      throwParseError(line);
    }
    ++pos;
    const auto uid = UID(domain, id);
    if (index.exists(uid)) {
      index.get(uid).name = line.substr(pos);
      if (Log::get().level == Log::Level::DEBUG) {
        std::stringstream buf;
        buf << "Loaded sequence " << index.get(uid);
        Log::get().debug(buf.str());
      }
    }
  }
}

void SequenceLoader::loadOffsets(const std::string &folder, char domain) {
  const std::string path = folder + "offsets";
  Log::get().debug("Loading sequence offsets from \"" + path + "\"");
  std::map<UID, std::string> entries;
  OeisList::loadMapWithComments(path, entries);
  for (const auto &entry : entries) {
    const UID id = entry.first;
    if (index.exists(id)) {
      index.get(id).offset = std::stoll(entry.second);
    }
  }
}

bool SequenceLoader::checkFolderDomain(std::string &folder, char domain) {
  if (folder.back() != '/' && folder.back() != '\\') {
    folder += FILE_SEP;
  }
  if (!isDir(folder)) {
    Log::get().error("Sequence folder not found: " + folder, true);
  }
  if (domain < 'A' || domain > 'Z') {
    Log::get().error("Invalid sequence domain: " + std::string(1, domain),
                     true);
  }
  bool found = false;
  for (size_t i = 0; i < folders.size(); i++) {
    if (folders[i] == folder) {
      if (domains[i] != domain) {
        Log::get().error("Conflicting domains for folder " + folder + ": " +
                             std::string(1, domains[i]) + " vs. " +
                             std::string(1, domain),
                         true);
      }
      found = true;
    }
    if (domains[i] == domain) {
      if (folders[i] != folder) {
        Log::get().error("Conflicting folders for domain " +
                             std::string(1, domain) + ": " + folders[i] +
                             " vs. " + folder,
                         true);
      }
      found = true;
    }
  }
  if (!found) {
    folders.push_back(folder);
    domains.push_back(domain);
  }
  return !found;
}

void SequenceLoader::checkConsistency() const {
  Log::get().debug("Checking sequence data consistency");
  size_t num_seqs = 0;
  for (const auto &s : index) {
    Log::get().debug("Checking consistency of " + s.to_string());
    if (s.id.empty()) {
      Log::get().error("Empty sequence ID", true);
    }
    if (s.name.empty()) {
      Log::get().error("Missing name for sequence " + s.id.string(), true);
    }
    if (s.existingNumTerms() < min_num_terms) {
      Log::get().error("Not enough terms for sequence " + s.id.string() + " (" +
                           std::to_string(s.existingNumTerms()) + "<" +
                           std::to_string(min_num_terms) + ")",
                       true);
    }
    num_seqs++;
  }
  if (num_seqs != num_loaded) {
    Log::get().error(
        "Inconsistent number of sequences: " + std::to_string(num_seqs) +
            "!=" + std::to_string(num_loaded),
        true);
  }
  Log::get().debug("Sequence data consistency check passed");
}
