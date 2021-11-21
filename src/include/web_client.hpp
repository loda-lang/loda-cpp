#pragma once

#include <string>

class WebClient {
 public:
  static bool get(const std::string &url, const std::string &local_path,
                  bool silent = false, bool fail_on_error = true);

  static bool postFile(const std::string &url, const std::string &file_path,
                       const std::string &auth = std::string());

 private:
  static int64_t WEB_CLIENT_TYPE;
  static void initWebClient();
};
