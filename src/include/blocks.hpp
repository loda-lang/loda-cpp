#pragma once

#include <map>
#include <set>

#include "program.hpp"

class Blocks {
 public:
  class Interface {
   public:
    Interface();

    Interface(const Program& p);

    void extend(const Operation& op);

    void clear();

    std::set<int64_t> inputs;
    std::set<int64_t> outputs;
    std::set<int64_t> all;
  };

  class Collector {
   public:
    void add(const Program& p);

    Blocks finalize();

    bool empty() const;

   private:
    Interface interface;  // cached
    std::map<Program, size_t> blocks;
  };

  void load(const std::string& path);

  void save(const std::string& path);

  Program getBlock(size_t index) const;

  Program list;
  std::vector<size_t> offsets;
  std::vector<double> rates;

 private:
  friend class Collector;

  void initRatesAndOffsets();
};
