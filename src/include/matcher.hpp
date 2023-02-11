#pragma once

#include <memory>
#include <unordered_set>

#include "extender.hpp"
#include "number.hpp"
#include "program.hpp"
#include "reducer.hpp"

class Matcher {
 public:
  using seq_programs_t = std::vector<std::pair<size_t, Program>>;

  using UPtr = std::unique_ptr<Matcher>;

  class Config {
   public:
    std::string type;
    bool backoff;
  };

  class Factory {
   public:
    static Matcher::UPtr create(const Matcher::Config &config);
  };

  virtual ~Matcher() {}

  virtual void insert(const Sequence &norm_seq, size_t id) = 0;

  virtual void remove(const Sequence &norm_seq, size_t id) = 0;

  virtual void match(const Program &p, const Sequence &norm_seq,
                     seq_programs_t &result) const = 0;

  virtual const std::string &getName() const = 0;

  virtual double getCompationRatio() const = 0;

  bool has_memory = true;
};

template <class T>
class AbstractMatcher : public Matcher {
 public:
  AbstractMatcher(const std::string &name, bool backoff)
      : name(name), backoff(backoff) {}

  virtual ~AbstractMatcher() {}

  virtual void insert(const Sequence &norm_seq, size_t id) override;

  virtual void remove(const Sequence &norm_seq, size_t id) override;

  virtual void match(const Program &p, const Sequence &norm_seq,
                     seq_programs_t &result) const override;

  virtual const std::string &getName() const override { return name; }

  virtual double getCompationRatio() const override {
    return 100.0 - (100.0 * ids.size() / std::max<size_t>(data.size(), 1));
  }

 protected:
  virtual std::pair<Sequence, T> reduce(const Sequence &seq,
                                        bool match) const = 0;

  virtual bool extend(Program &p, T base, T gen) const = 0;

 private:
  bool shouldMatchSequence(const Sequence &seq) const;

  std::string name;
  SequenceToIdsMap ids;
  std::unordered_map<size_t, T> data;
  mutable std::unordered_set<Sequence, SequenceHasher> match_attempts;
  bool backoff;
};

class DirectMatcher : public AbstractMatcher<int> {
 public:
  DirectMatcher(bool backoff) : AbstractMatcher("direct", backoff) {}

  virtual ~DirectMatcher() {}

 protected:
  virtual std::pair<Sequence, int> reduce(const Sequence &seq,
                                          bool match) const override;

  virtual bool extend(Program &p, int base, int gen) const override;
};

class LinearMatcher : public AbstractMatcher<line_t> {
 public:
  LinearMatcher(bool backoff) : AbstractMatcher("linear1", backoff) {}

  virtual ~LinearMatcher() {}

 protected:
  virtual std::pair<Sequence, line_t> reduce(const Sequence &seq,
                                             bool match) const override;

  virtual bool extend(Program &p, line_t base, line_t gen) const override;
};

class LinearMatcher2 : public AbstractMatcher<line_t> {
 public:
  LinearMatcher2(bool backoff) : AbstractMatcher("linear2", backoff) {}

  virtual ~LinearMatcher2() {}

 protected:
  virtual std::pair<Sequence, line_t> reduce(const Sequence &seq,
                                             bool match) const override;

  virtual bool extend(Program &p, line_t base, line_t gen) const override;
};

class DeltaMatcher : public AbstractMatcher<delta_t> {
 public:
  static const int64_t MAX_DELTA;

  DeltaMatcher(bool backoff) : AbstractMatcher("delta", backoff) {}

  virtual ~DeltaMatcher() {}

 protected:
  virtual std::pair<Sequence, delta_t> reduce(const Sequence &seq,
                                              bool match) const override;

  virtual bool extend(Program &p, delta_t base, delta_t gen) const override;
};

class DigitMatcher : public AbstractMatcher<int64_t> {
 public:
  DigitMatcher(const std::string &name, int64_t num_digits, bool backoff)
      : AbstractMatcher(name, backoff),
        num_digits(num_digits),
        num_digits_big(num_digits) {}

  virtual ~DigitMatcher() {}

 protected:
  virtual std::pair<Sequence, int64_t> reduce(const Sequence &seq,
                                              bool match) const override;

  virtual bool extend(Program &p, int64_t base, int64_t gen) const override;

 private:
  const int64_t num_digits;
  const Number num_digits_big;
};
