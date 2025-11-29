#include "sys/web_client.hpp"

#include <cstring>
#include <fstream>

#include <curl/curl.h>

// Undefine Windows macros that conflict with our code
#ifdef ERROR
#undef ERROR
#endif

#include "sys/file.hpp"
#include "sys/log.hpp"

int64_t WebClient::WEB_CLIENT_TYPE = 0;

void WebClient::initWebClient() {
  if (WEB_CLIENT_TYPE == 0) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    WEB_CLIENT_TYPE = 1;
    // Note: curl_global_cleanup() is intentionally not called as it's not
    // thread-safe and the library remains usable for the lifetime of the
    // program. This is a common pattern for libcurl usage.
  }
}

namespace {

// Callback function to write data to a file
size_t writeFileCallback(void* contents, size_t size, size_t nmemb,
                         void* userp) {
  std::ofstream* file = static_cast<std::ofstream*>(userp);
  size_t total_size = size * nmemb;
  file->write(static_cast<char*>(contents), total_size);
  return total_size;
}

// Callback function to write data to a string
size_t writeStringCallback(void* contents, size_t size, size_t nmemb,
                           void* userp) {
  std::string* str = static_cast<std::string*>(userp);
  size_t total_size = size * nmemb;
  str->append(static_cast<char*>(contents), total_size);
  return total_size;
}

// Callback function to read data from a file
size_t readFileCallback(void* ptr, size_t size, size_t nmemb, void* userp) {
  std::ifstream* file = static_cast<std::ifstream*>(userp);
  size_t total_size = size * nmemb;
  file->read(static_cast<char*>(ptr), total_size);
  return file->gcount();
}

}  // namespace

bool WebClient::get(const std::string& url, const std::string& local_path,
                    bool silent, bool fail_on_error, bool insecure) {
  initWebClient();

  CURL* curl = curl_easy_init();
  if (!curl) {
    if (fail_on_error) {
      Log::get().error("Failed to initialize libcurl", true);
    }
    return false;
  }

  std::ofstream file(local_path, std::ios::binary);
  if (!file.is_open()) {
    curl_easy_cleanup(curl);
    if (fail_on_error) {
      Log::get().error("Failed to open file for writing: " + local_path, true);
    }
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFileCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
  if (insecure) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  }

  CURLcode res = curl_easy_perform(curl);
  file.close();

  if (res != CURLE_OK) {
    std::remove(local_path.c_str());
    curl_easy_cleanup(curl);
    if (fail_on_error) {
      Log::get().error("Error fetching " + url + ": " +
                           std::string(curl_easy_strerror(res)),
                       true);
    }
    return false;
  }

  curl_easy_cleanup(curl);
  if (!silent) {
    Log::get().info("Fetched " + url);
  }
  return true;
}

bool WebClient::postFile(const std::string& url, const std::string& file_path,
                         const std::string& auth,
                         const std::vector<std::string>& headers,
                         bool enable_debug) {
  initWebClient();

  CURL* curl = curl_easy_init();
  if (!curl) {
    if (enable_debug) {
      Log::get().error("Failed to initialize libcurl", false);
    }
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  if (!auth.empty()) {
    curl_easy_setopt(curl, CURLOPT_USERPWD, auth.c_str());
  }

  struct curl_slist* header_list = nullptr;
  for (const auto& header : headers) {
    header_list = curl_slist_append(header_list, header.c_str());
  }
  if (header_list) {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
  }

  std::ifstream file;
  if (!file_path.empty()) {
    file.open(file_path, std::ios::binary);
    if (!file.is_open()) {
      curl_slist_free_all(header_list);
      curl_easy_cleanup(curl);
      if (enable_debug) {
        Log::get().error("Failed to open file: " + file_path, false);
      }
      return false;
    }

    file.seekg(0, std::ios::end);
    std::streampos pos = file.tellg();
    if (pos == std::streampos(-1)) {
      file.close();
      curl_slist_free_all(header_list);
      curl_easy_cleanup(curl);
      if (enable_debug) {
        Log::get().error("Failed to determine file size: " + file_path, false);
      }
      return false;
    }
    size_t file_size = static_cast<size_t>(pos);
    file.seekg(0, std::ios::beg);

    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, readFileCallback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &file);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(file_size));
  } else {
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
  }

  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

  std::string response;
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStringCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  if (enable_debug) {
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    Log::get().info("Posting to URL: " + url);
  }

  CURLcode res = curl_easy_perform(curl);

  if (file.is_open()) {
    file.close();
  }
  curl_slist_free_all(header_list);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    if (enable_debug) {
      Log::get().error("Error posting to " + url + ": " +
                           std::string(curl_easy_strerror(res)),
                       false);
    }
    return false;
  }

  return true;
}

jute::jValue WebClient::getJson(const std::string& url) {
  static int64_t request_counter = 0;
  const std::string tmp =
      getTmpDir() + "web_json_" + std::to_string(request_counter++) + ".json";
  if (!get(url, tmp, false, false)) {
    std::remove(tmp.c_str());
    throw std::runtime_error("Failed to fetch JSON from URL: " + url);
  }
  jute::jValue json;
  try {
    json = jute::parser::parse_file(tmp);
  } catch (const std::exception& e) {
    std::remove(tmp.c_str());
    throw std::runtime_error("Failed to parse JSON response: " +
                             std::string(e.what()));
  }
  std::remove(tmp.c_str());
  return json;
}
