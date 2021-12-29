#include "config.hpp"

#include "file.hpp"
#include "jute.h"
#include "setup.hpp"

int64_t get_jint(jute::jValue &v, const std::string &key, int64_t def) {
  if (v[key].get_type() == jute::jType::JNUMBER) {
    return v[key].as_int();
  }
  return def;
}

double get_jdouble(jute::jValue &v, const std::string &key, double def) {
  if (v[key].get_type() == jute::jType::JNUMBER) {
    return v[key].as_double();
  }
  return def;
}

bool get_jbool(jute::jValue &v, const std::string &key, bool def) {
  if (v[key].get_type() == jute::jType::JBOOLEAN) {
    return v[key].as_bool();
  }
  return def;
}

std::string get_template(std::string t) {
  static const std::string h(
      "$LODA_HOME/programs/");  // TODO: use proper variable replacing
  if (t.rfind(h, 0) == 0) {
    t = t.substr(h.size());
    t = Setup::getProgramsHome() + t;
  }
  if (FILE_SEP != '/') {
    std::replace(t.begin(), t.end(), '/', FILE_SEP);
  }
  return t;
}

std::vector<Generator::Config> loadGeneratorConfigs(
    const std::string &miner, jute::jValue &gens,
    const std::unordered_set<std::string> &names) {
  std::vector<Generator::Config> generators;
  for (int i = 0; i < gens.size(); i++) {
    auto g = gens[i];
    auto name = g["name"].as_string();
    if (names.find(name) == names.end()) {
      continue;
    }
    Generator::Config c;
    c.version = get_jint(g, "version", 1);
    c.miner = miner;
    c.length = get_jint(g, "length", 20);
    c.max_constant = get_jint(g, "maxConstant", 4);
    c.max_index = get_jint(g, "maxIndex", 4);
    c.mutation_rate = get_jdouble(g, "mutationRate", 0.3);
    c.loops = get_jbool(g, "loops", true);
    c.calls = get_jbool(g, "calls", true);
    c.indirect_access = get_jbool(g, "indirectAccess", false);
    auto t = g["template"].get_type();
    switch (t) {
      case jute::jType::JSTRING: {
        c.templates.push_back(get_template(g["template"].as_string()));
        break;
      }
      case jute::jType::JARRAY: {
        auto a = g["template"];
        for (int i = 0; i < a.size(); i++) {
          if (a[i].get_type() == jute::jType::JSTRING) {
            c.templates.push_back(get_template(a[i].as_string()));
          }
        }
        break;
      }
      case jute::jType::JNULL:
      case jute::jType::JUNKNOWN: {
        break;
      }
      default: {
        throw std::runtime_error("unexpected template value: " +
                                 std::to_string(t));
        break;
      }
    }
    generators.push_back(c);
  }
  return generators;
}

Miner::Config ConfigLoader::load(const Settings &settings) {
  const std::string loda_config = Setup::getMinersConfig();
  Log::get().debug("Loading miner config \"" + settings.miner + "\" from " +
                   loda_config);
  Miner::Config config;

  auto str = getFileAsString(loda_config);
  auto spec = jute::parser::parse(str);
  auto miners = spec["miners"];

  int index = -1;
  if (!settings.miner.empty() &&
      std::find_if(settings.miner.begin(), settings.miner.end(),
                   [](unsigned char c) { return !std::isdigit(c); }) ==
          settings.miner.end()) {
    index = stoi(settings.miner) % miners.size();
  }

  bool found = false;
  for (int i = 0; i < miners.size(); i++) {
    auto m = miners[i];
    auto name = m["name"].as_string();
    if (name == settings.miner || i == index) {
      config.name = name;
      auto overwrite_mode = m["overwrite"].as_string();
      if (overwrite_mode == "none") {
        config.overwrite_mode = OverwriteMode::NONE;
      } else if (overwrite_mode == "all") {
        config.overwrite_mode = OverwriteMode::ALL;
      } else if (overwrite_mode == "auto") {
        config.overwrite_mode = OverwriteMode::AUTO;
      } else {
        throw std::runtime_error("Unknown overwrite mode: " + overwrite_mode);
      }

      // load matcher configs
      bool backoff = get_jbool(m, "backoff", true);
      auto matchers = m["matchers"];
      for (int j = 0; j < matchers.size(); j++) {
        Matcher::Config mc;
        mc.backoff = backoff;
        mc.type = matchers[j].as_string();
        config.matchers.push_back(mc);
      }

      // load generator configs
      auto gen_names = m["generators"];
      std::unordered_set<std::string> names;
      for (int j = 0; j < gen_names.size(); j++) {
        names.insert(gen_names[j].as_string());
      }
      auto gens = spec["generators"];
      config.generators = loadGeneratorConfigs(name, gens, names);

      // done
      found = true;
      break;
    }
  }
  if (!found) {
    Log::get().error("Miner config not found: " + settings.miner, true);
  }
  Log::get().debug("Finished loading miner config \"" + settings.miner +
                   "\" from " + loda_config + " with " +
                   std::to_string(config.generators.size()) + " generators");
  return config;
}

/*
TODO: handle these settings via setup

  std::cout << std::endl << "=== Environment variables ===" << std::endl;
  std::cout << "LODA_UPDATE_INTERVAL     Update interval for OEIS metadata in "
               "days (default: " +
                   std::to_string(settings.update_interval_in_days) + ")"
            << std::endl;
  std::cout << "LODA_MAX_CYCLES          Maximum number of interpreter cycles "
               "(same as -c)"
            << std::endl;
  std::cout << "LODA_MAX_MEMORY          Maximum number of used memory cells "
               "(same as -m)"
            << std::endl;
  std::cout
      << "LODA_MAX_PHYSICAL_MEMORY Maximum physical memory in MB (same as -p)"
      << std::endl;
  std::cout
      << "LODA_SLACK_ALERTS        Enable alerts on Slack (default: false)"
      << std::endl;
  std::cout
      << "LODA_TWEET_ALERTS        Enable alerts on Twitter (default: false)"
      << std::endl;
  std::cout << "LODA_INFLUXDB_HOST       InfluxDB host name (URL) for "
               "publishing metrics"
            << std::endl;
  std::cout << "LODA_INFLUXDB_AUTH       InfluxDB authentication info "
               "('user:password' format)"
            << std::endl;
*/
