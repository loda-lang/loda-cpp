#include "seq/sequence_index.hpp"

bool SequenceIndex::exists(UID id) const {
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

const ManagedSequence& SequenceIndex::get(UID uid) const {
  return at(uid.domain()).at(uid.number());
}

ManagedSequence& SequenceIndex::get(UID uid) {
  return (*this)[uid.domain()][uid.number()];
}

void SequenceIndex::add(ManagedSequence&& seq) {
  auto& seqs = (*this)[seq.id.domain()];
  auto index = seq.id.number();
  if (index >= static_cast<int64_t>(seqs.size())) {
    seqs.resize(static_cast<int64_t>(1.5 * index) + 1);
  }
  seqs[index] = std::move(seq);
}
