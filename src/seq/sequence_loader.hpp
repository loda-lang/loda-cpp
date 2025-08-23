#pragma once

#include "seq/sequence_index.hpp"

class SequenceLoader {
 public:
  SequenceLoader(SequenceIndex& index, size_t min_num_terms);

  void load();

  size_t getNumLoaded() const { return num_loaded; }
  size_t getNumTotal() const { return num_total; }

 private:
  void loadData();
  void loadNames();
  void loadOffsets();
  void checkConsistency() const;

  SequenceIndex& index;
  const size_t min_num_terms;
  size_t num_loaded;
  size_t num_total;
};
