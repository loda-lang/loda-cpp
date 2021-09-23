#include "metrics.hpp"

#include <fstream>
#include <random>

#include "file.hpp"
#include "setup.hpp"
#include "util.hpp"

Metrics::Metrics()
    : publish_interval(Setup::getAdvancedConfigInt(
          "LODA_METRICS_PUBLISH_INTERVAL", 60)),  // magic number
      notified(false) {
  host = Setup::getAdvancedConfig("LODA_INFLUXDB_HOST");
  if (!host.empty()) {
    auth = Setup::getAdvancedConfig("LODA_INFLUXDB_AUTH");
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

#ifdef _WIN64
  const std::string file_name =
      "loda_metrics_" + std::to_string(tmp_file_id) + ".txt";
#else
  const std::string file_name =
      "/tmp/loda_metrics_" + std::to_string(tmp_file_id) + ".txt";
#endif
  std::ofstream out(file_name);
  for (auto &entry : entries) {
    out << entry.field;
    for (auto &l : entry.labels) {
      out << "," << l.first << "=" << l.second;
    }
    out << " value=" << entry.value << "\n";
  }
  out.close();
  if (!Http::postFile(host + "/write?db=loda", file_name, auth)) {
    Log::get().error("Error publishing metrics", false);
  }
  std::remove(file_name.c_str());
}
