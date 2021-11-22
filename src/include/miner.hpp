#pragma once

#include <memory>

#include "api_client.hpp"
#include "generator.hpp"
#include "matcher.hpp"
#include "number.hpp"
#include "oeis_manager.hpp"
#include "optimizer.hpp"
#include "program.hpp"
#include "util.hpp"

class Miner {
 public:
  class Config {
   public:
    std::string name;
    OverwriteMode overwrite_mode;
    std::vector<Generator::Config> generators;
    std::vector<Matcher::Config> matchers;
  };

  Miner(const Settings &settings);

  void mine();

  void submit(const std::string &id, const std::string &path);

 private:
  void reload(bool load_generators, bool force_overwrite = false);

  void ensureSubmittedBy(Program &program);

  void setSubmittedBy(Program &program);

  static const std::string ANONYMOUS;
  const Settings &settings;
  std::unique_ptr<ApiClient> api_client;
  std::unique_ptr<OeisManager> manager;
  std::unique_ptr<MultiGenerator> multi_generator;
};
