#include "sys/metrics.hpp"

#include <algorithm>
#include <fstream>

#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"
#include "sys/web_client.hpp"

Metrics::Metrics()
    : publish_interval(Setup::getSetupInt("LODA_METRICS_PUBLISH_INTERVAL",
                                          300)),  // magic number
      notified(false) {
  host = Setup::getSetupValue("LODA_INFLUXDB_HOST");
  if (!host.empty()) {
    auth = Setup::getSetupValue("LODA_INFLUXDB_AUTH");
  }
  tmp_file_id = Random::get().gen() % 1000;
}

Metrics &Metrics::get() {
  static Metrics metrics;
  return metrics;
}

void Metrics::write(const std::vector<Entry> &entries) const {
  if (host.empty()) {
    return;
  }
  if (!notified) {
    Log::get().debug("Publishing metrics to InfluxDB");
    notified = true;
  }
  // attention: curl sometimes has problems with absolute paths.
  // so we use a relative path here!
  const std::string tmp_file =
      "loda_metrics_" + std::to_string(tmp_file_id) + ".txt";
  std::ofstream out(tmp_file);
  for (const auto &entry : entries) {
    out << entry.field;
    for (const auto &l : entry.labels) {
      auto v = l.second;
      replaceAll(v, " ", "\\ ");
      out << "," << l.first << "=" << v;
    }
    out << " value=" << entry.value << "\n";
  }
  out.close();
  const std::string url = host + "/write?db=loda";
  if (!WebClient::postFile(url, tmp_file, auth)) {
    WebClient::postFile(url, tmp_file, auth, true);  // for debugging
    Log::get().error("Error publishing metrics", false);
  }
  std::remove(tmp_file.c_str());
}
