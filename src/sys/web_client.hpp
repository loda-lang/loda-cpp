#pragma once

#include <string>
#include <vector>

class WebClient {
 public:
  static bool get(const std::string& url, const std::string& local_path,
                  bool silent = false, bool fail_on_error = true,
                  bool insecure = false);

  static bool postFile(const std::string& url, const std::string& file_path,
                       const std::string& auth = std::string(),
                       const std::vector<std::string>& headers = {},
                       bool enable_debug = false);

 private:
  static int64_t WEB_CLIENT_TYPE;
  static void initWebClient();
};
