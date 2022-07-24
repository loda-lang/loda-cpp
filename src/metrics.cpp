#include "metrics.hpp"

#include <algorithm>
#include <fstream>
#include <random>

#include "file.hpp"
#include "log.hpp"
#include "setup.hpp"
#include "util.hpp"
#include "web_client.hpp"

Metrics::Metrics()
    : publish_interval(Setup::getSetupInt("LODA_METRICS_PUBLISH_INTERVAL",
                                          300)),  // magic number
      notified(false) {
  host = Setup::getSetupValue("LODA_INFLUXDB_HOST");
  if (!host.empty()) {
    auth = Setup::getSetupValue("LODA_INFLUXDB_AUTH");
  }
  std::random_device rand;
  tmp_file_id = rand() % 1000;
}

Metrics &Metrics::get() {
  static Metrics metrics;
  return metrics;
}

void Metrics::write(const std::vector<Entry> entries) const {
  if (Metrics::host.empty()) {
    return;
  }
  if (!notified) {
    Log::get().debug("Publishing metrics to InfluxDB");
    notified = true;
  }
  // attention: curl sometimes has problems with absolute paths.
  // so we use a relative path here!
  const std::string file_name =
      "loda_metrics_" + std::to_string(tmp_file_id) + ".txt";
  std::ofstream out(file_name);
  for (auto &entry : entries) {
    out << entry.field;
    for (auto &l : entry.labels) {
      auto v = l.second;
      replaceAll(v, " ", "\\ ");
      out << "," << l.first << "=" << v;
    }
    out << " value=" << entry.value << "\n";
  }
  out.close();
  const std::string url = host + "/write?db=loda";
  if (WebClient::postFile(url, file_name, auth)) {
    std::remove(file_name.c_str());
  } else {
    WebClient::postFile(url, file_name, auth, true);  // for debugging
    Log::get().error("Error publishing metrics", false);
  }
}
