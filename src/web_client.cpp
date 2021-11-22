#include "web_client.hpp"

#include "file.hpp"
#include "util.hpp"

enum WebClientType { WC_UNKNOWN = 0, WC_CURL = 1, WC_WGET = 2 };

int64_t WebClient::WEB_CLIENT_TYPE = WC_UNKNOWN;

void WebClient::initWebClient() {
  if (WEB_CLIENT_TYPE == WC_UNKNOWN) {
    const std::string curl_cmd = "curl --version " + getNullRedirect();
    const std::string wget_cmd = "wget --version " + getNullRedirect();
    if (system(curl_cmd.c_str()) == 0) {
      WEB_CLIENT_TYPE = WC_CURL;
    } else if (system(wget_cmd.c_str()) == 0) {
      WEB_CLIENT_TYPE = WC_WGET;
    } else {
      Log::get().error("No web client found. Please install curl or wget.",
                       true);
    }
  }
}

bool WebClient::get(const std::string &url, const std::string &local_path,
                    bool silent, bool fail_on_error) {
  initWebClient();
  std::string cmd;
  switch (WEB_CLIENT_TYPE) {
    case WC_CURL:
      cmd = "curl -fsSLo \"" + local_path + "\" " + url;
      break;
    case WC_WGET:
      cmd =
          "wget -q --no-use-server-timestamps -O \"" + local_path + "\" " + url;
      break;
    default:
      Log::get().error("Unsupported web client for GET request", true);
  }
  if (system(cmd.c_str()) != 0) {
    std::remove(local_path.c_str());
    if (fail_on_error) {
      Log::get().error("Error fetching " + url, true);
    } else {
      return false;
    }
  }
  if (!silent) {
    Log::get().info("Fetched " + url);
  }
  return true;
}

bool WebClient::postFile(const std::string &url, const std::string &file_path,
                         const std::string &auth) {
  initWebClient();
  std::string cmd;
  switch (WEB_CLIENT_TYPE) {
    case WC_CURL: {
      cmd = "curl -fsSL";
      if (!auth.empty()) {
        cmd += " -u " + auth;
      }
      cmd += " -X POST '" + url + "' --data-binary @\"" + file_path + "\" " +
             getNullRedirect();
      break;
    }
    case WC_WGET: {
      cmd = "wget -q";
      if (!auth.empty()) {
        auto colon = auth.find(':');
        cmd += " --user '" + auth.substr(0, colon) + "' --password '" +
               auth.substr(colon + 1) + "'";
      }
      cmd +=
          " --post-file \"" + file_path + "\" " + url + " " + getNullRedirect();
      break;
    }
    default:
      Log::get().error("Unsupported web client for POST request", true);
  }
  auto exit_code = system(cmd.c_str());
  return (exit_code == 0);
}
