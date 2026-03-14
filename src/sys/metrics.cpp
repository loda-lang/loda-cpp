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

Metrics& Metrics::get() {
  static Metrics metrics;
  return metrics;
}

void Metrics::write(const std::vector<Entry>& entries) const {
  if (host.empty()) {
    return;
  }
  if (!notified) {
    Log::get().debug("Publishing metrics to InfluxDB");
    notified = true;
  }
  std::string content;
  for (const auto& entry : entries) {
    content += entry.field;
    for (const auto& l : entry.labels) {
      auto v = l.second;
      replaceAll(v, " ", "\\ ");
      content += "," + l.first + "=" + v;
    }
    content += " value=" + std::to_string(entry.value) + "\n";
  }
  const std::string url = host + "/write?db=loda";
  if (!WebClient::postContent(url, content, auth)) {
    WebClient::postContent(url, content, auth, {}, true);  // for debugging
    Log::get().error("Error publishing metrics", false);
  }
}
