#include "mine/config.hpp"

#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"

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
    c.version = getJInt(g, "version", 1);
    c.miner = miner;
    c.length = getJInt(g, "length", 20);
    c.max_constant = getJInt(g, "maxConstant", 4);
    c.max_index = getJInt(g, "maxIndex", 4);
    c.mutation_rate = getJDouble(g, "mutationRate", 0.3);
    c.loops = getJBool(g, "loops", true);
    c.calls = getJBool(g, "calls", true);
    c.indirect_access = getJBool(g, "indirectAccess", false);
    if (g["batchFile"].get_type() == jute::jType::JSTRING) {
      c.batch_file = g["batchFile"].as_string();
    }
    auto t = g["template"].get_type();
    switch (t) {
      case jute::jType::JSTRING: {
        c.templates.push_back(get_template(g["template"].as_string()));
        break;
      }
      case jute::jType::JARRAY: {
        auto a = g["template"];
        for (int j = 0; j < a.size(); j++) {
          if (a[j].get_type() == jute::jType::JSTRING) {
            c.templates.push_back(get_template(a[j].as_string()));
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
      }
    }
    generators.push_back(c);
  }
  return generators;
}

Miner::Config ConfigLoader::load(const Settings &settings) {
  const std::string loda_config = Setup::getMinersConfig();
  Miner::Config config;

  auto str = getFileAsString(loda_config);
  auto spec = jute::parser::parse(str);
  auto all = spec["miners"];

  // filter based on "enabled" flag
  std::vector<jute::jValue> miners;
  for (int i = 0; i < all.size(); i++) {
    auto m = all[i];
    if (getJBool(m, "enabled", true)) {
      miners.push_back(m);
    }
  }

  // determine which profile to use
  std::string profile = "0";  // default: first profile in config
  if (!settings.miner_profile.empty()) {
    profile = settings.miner_profile;
  }
  int index = -1;
  if (!profile.empty() &&
      std::find_if(profile.begin(), profile.end(), [](unsigned char c) {
        return !std::isdigit(c);
      }) == profile.end()) {
    index = stoi(profile) % miners.size();
  }

  bool found = false;
  for (int i = 0; i < static_cast<int>(miners.size()); i++) {
    auto m = miners[i];
    auto name = m["name"].as_string();
    if (name == profile || i == index) {
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
      auto validation_mode = m["validation"].as_string();
      if (validation_mode == "" || validation_mode == "extended") {
        config.validation_mode = ValidationMode::EXTENDED;  // default
      } else if (validation_mode == "basic") {
        config.validation_mode = ValidationMode::BASIC;
      }
      config.domains = m["domains"].as_string();
      if (config.domains.empty()) {
        config.domains = "A";
      }
      for (char d : config.domains) {
        if (d < 'A' || d > 'Z') {
          throw std::runtime_error("Invalid domain: " + std::string(1, d));
        }
      }

      // load matcher configs
      bool backoff = getJBool(m, "backoff", true);
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
    Log::get().error("Miner config not found or disabled: " + profile, true);
  }
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
