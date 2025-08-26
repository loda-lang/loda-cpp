#pragma once

#include "seq/seq_index.hpp"

class SequenceLoader {
 public:
  SequenceLoader(SequenceIndex& index, size_t min_num_terms);

  void load(std::string folder, char domain);

  void checkConsistency() const;

  size_t getNumLoaded() const { return num_loaded; }
  size_t getNumTotal() const { return num_total; }

 private:
  void loadData(const std::string& folder, char domain);
  void loadNames(const std::string& folder, char domain);
  void loadOffsets(const std::string& folder, char domain);

  bool checkFolderDomain(std::string& folder, char domain);

  SequenceIndex& index;
  const size_t min_num_terms;
  size_t num_loaded;
  size_t num_total;
  std::vector<std::string> folders;
  std::vector<char> domains;
};
