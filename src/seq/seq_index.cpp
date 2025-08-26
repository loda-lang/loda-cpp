#include "seq/seq_index.hpp"

bool SequenceIndex::exists(UID id) const {
  auto it = data.find(id.domain());
  if (it == data.end()) {
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
  return data.at(uid.domain()).at(uid.number());
}

ManagedSequence& SequenceIndex::get(UID uid) {
  return data[uid.domain()][uid.number()];
}

void SequenceIndex::add(ManagedSequence&& seq) {
  auto& seqs = data[seq.id.domain()];
  auto index = seq.id.number();
  if (index >= static_cast<int64_t>(seqs.size())) {
    seqs.resize(static_cast<int64_t>(1.5 * index) + 1);
  }
  seqs[index] = std::move(seq);
}

using const_iterator = SequenceIndex::const_iterator;
using outer_iter_t = const_iterator::outer_iter_t;
using inner_iter_t = const_iterator::inner_iter_t;

const_iterator::const_iterator(outer_iter_t outer, outer_iter_t outer_end)
    : outer_it(outer), outer_end(outer_end) {
  if (outer_it != outer_end) {
    inner_it = outer_it->second.begin();
    advance_to_valid();
  }
}

const ManagedSequence& const_iterator::operator*() const { return *inner_it; }

const ManagedSequence* const_iterator::operator->() const {
  return &(*inner_it);
}

const_iterator& const_iterator::operator++() {
  ++inner_it;
  advance_to_valid();
  return *this;
}

bool const_iterator::operator==(const const_iterator& other) const {
  return outer_it == other.outer_it &&
         (outer_it == outer_end || inner_it == other.inner_it);
}

bool const_iterator::operator!=(const const_iterator& other) const {
  return !(*this == other);
}

void const_iterator::advance_to_valid() {
  while (outer_it != outer_end && inner_it == outer_it->second.end()) {
    ++outer_it;
    if (outer_it != outer_end) {
      inner_it = outer_it->second.begin();
    }
  }
  // skip empty slots
  while (outer_it != outer_end && inner_it != outer_it->second.end() &&
         inner_it->id.empty()) {
    ++inner_it;
    if (inner_it == outer_it->second.end()) {
      ++outer_it;
      if (outer_it != outer_end) {
        inner_it = outer_it->second.begin();
      }
    }
  }
}

const_iterator SequenceIndex::begin() const {
  return const_iterator(data.begin(), data.end());
}

const_iterator SequenceIndex::end() const {
  return const_iterator(data.end(), data.end());
}
