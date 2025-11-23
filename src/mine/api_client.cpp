#include "mine/api_client.hpp"

#include <fstream>
#include <sstream>
#include <thread>

#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "mine/submission.hpp"
#include "sys/file.hpp"
#include "sys/git.hpp"
#include "sys/jute.h"
#include "sys/log.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"
#include "sys/web_client.hpp"

ApiClient::ApiClient()
    : client_id(Random::get().gen() % 100000),
      session_id(0),
      start(0),
      count(0),
      fetched_oeis_files(0),
      last_oeis_time(std::chrono::steady_clock::now()),
      printed_throttling_warning(false) {
  auto server = Setup::getSetupValue("LODA_API_SERVER");
  if (server.empty()) {
    server = "https://api.loda-lang.org/";
  } else {
    Log::get().info("Using configured API server: " + server);
  }
  if (server.back() != '/') {
    server += '/';
  }
  base_url = server + "miner/v1/";
  base_url_v2 = server + "v2/";
  oeis_fetch_direct = Setup::getSetupFlag("LODA_OEIS_FETCH_DIRECT", false);
}

ApiClient& ApiClient::getDefaultInstance() {
  static ApiClient api_client;
  return api_client;
}

void ApiClient::postProgram(const Program& program, size_t max_buffer) {
  // attention: curl sometimes has problems with absolute paths.
  // so we use a relative path here!
  const std::string tmp = "post_program_" + std::to_string(client_id) + ".asm";
  out_queue.push_back(program);
  while (!out_queue.empty()) {
    {
      std::ofstream out(tmp);
      ProgramUtil::print(out_queue.back(), out);
      out.close();
    }
    if (postProgram(tmp, out_queue.size() > max_buffer)) {
      out_queue.pop_back();
    } else {
      break;
    }
  }
  std::remove(tmp.c_str());
}

bool ApiClient::postProgram(const std::string& path, bool fail_on_error) {
  if (!isFile(path)) {
    Log::get().error("File not found: " + path, true);
  }
  const std::string url = base_url + "programs";
  if (!WebClient::postFile(url, path)) {
    const std::string msg("Cannot submit program to API server");
    if (fail_on_error) {
      if (!WebClient::postFile(url, path, {}, {}, true)) {
        Log::get().error(msg, true);
      }
    } else {
      Log::get().warn(msg);
    }
    return false;
  }
  return true;
}

void ApiClient::postCPUHour() {
  const auto tmp_file_id = Random::get().gen() % 1000;
  // attention: curl sometimes has problems with absolute paths.
  // so we use a relative path here!
  // TODO: extend WebClient::postFile to support content directly
  const std::string tmp_file =
      "loda_usage_" + std::to_string(tmp_file_id) + ".json";
  std::ofstream out(tmp_file);
  out << "{\"version\":\"" << Version::VERSION << "\", \"platform\":\""
      << Version::PLATFORM << "\", \"cpuHours\":1" << "}\n";
  out.close();
  const std::vector<std::string> headers = {"Content-Type: application/json"};
  const std::string url = base_url + "cpuhours";
  if (!WebClient::postFile(url, tmp_file, {}, headers)) {
    WebClient::postFile(url, tmp_file, {}, headers,
                        true);  // for debugging
    Log::get().error("Error reporting usage", false);
  }
  std::remove(tmp_file.c_str());
}

void ApiClient::getOeisFile(const std::string& filename,
                            const std::string& local_path) {
  // throttling
  if (fetched_oeis_files > 2) {
    int64_t secs = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::steady_clock::now() - last_oeis_time)
                       .count();
    if (secs < OEIS_THROTTLING_SECS) {
      if (!printed_throttling_warning) {
        Log::get().warn("Throttling download of OEIS files");
        printed_throttling_warning = true;
      }
      std::this_thread::sleep_for(
          std::chrono::seconds(OEIS_THROTTLING_SECS - secs));
    }
  }

  // fetch file
  std::string url, ext;
  const bool is_b_file = filename.front() == 'b';
  if (oeis_fetch_direct &&
      (is_b_file || filename == "names" || filename == "stripped")) {
    url = "https://www.oeis.org/";
    if (is_b_file) {
      auto id = filename.substr(1, 6);
      url += "A" + id + "/" + filename;
    } else {
      ext = ".gz";
      url += filename + ext;
    }
  } else {
    ext = ".gz";
    url = base_url + "oeis/" + filename + ext;
  }

  bool success = false;
  int64_t backoff_delay = OEIS_THROTTLING_SECS;
  for (int64_t i = 0; i < 5; i++) {
    if (i > 0) {
      Log::get().warn("Retrying fetch of " + url);
    }
    success = WebClient::get(url, local_path + ext, false, false);
    if (success) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::seconds(backoff_delay));
    backoff_delay *= 2;
  }
  if (success) {
    if (ext == ".gz") {
      Git::gunzip(local_path + ".gz", !is_b_file);
    }
    fetched_oeis_files++;
    last_oeis_time = std::chrono::steady_clock::now();
  } else {
    Log::get().error("Error fetching " + url, true);
  }
}

bool ApiClient::getProgram(int64_t index, const std::string& path) {
  std::remove(path.c_str());
  return WebClient::get(base_url + "programs/" + std::to_string(index), path,
                        false, false);
}

Submission ApiClient::getNextSubmission() {
  if (session_id == 0 || pages.empty()) {
    updateSessionV2();
  }
  Log::get().info("Fetching next submission");
  Submission submission;
  if (pages.empty()) {
    return submission;
  }
  if (in_queue_current_page.empty()) {
    // fetch next page
    const Page page = pages.back();
    pages.pop_back();
    std::stringstream endpoint;
    endpoint << "submissions?limit=" << page.count << "&skip=" << page.start
             << "&type=program";
    auto json = WebClient::getJson(base_url_v2 + endpoint.str());
    auto submissions = json["results"];
    if (submissions.get_type() != jute::JARRAY) {
      throw std::runtime_error(
          "Invalid JSON response: missing submissions array");
    }
    in_queue_current_page.clear();
    for (int i = 0; i < submissions.size(); i++) {
      auto submission_json = submissions[i];
      // Create submission from JSON (this will validate type and mode)
      Submission sub;
      try {
        sub = Submission::fromJson(submission_json);
      } catch (const std::exception& e) {
        Log::get().warn("Failed to parse submission: " + std::string(e.what()));
        continue;
      }
      if (sub.type != Submission::Type::PROGRAM) {
        continue;  // Skip non-program submissions
      }
      if (sub.content.empty()) {
        continue;  // Skip if no content
      }
      // Validate that the program can be parsed
      Program program = sub.toProgram();
      if (!program.ops.empty()) {
        in_queue_current_page.push_back(sub);
      }
    }
    std::shuffle(in_queue_current_page.begin(), in_queue_current_page.end(),
                 Random::get().gen);
  }
  if (in_queue_current_page.empty()) {
    return submission;
  }
  submission = in_queue_current_page.back();
  in_queue_current_page.pop_back();
  return submission;
}

Program ApiClient::getNextProgram() {
  if (session_id == 0 || in_queue.empty()) {
    updateSession();
  }
  Program program;
  if (in_queue.empty()) {
    return program;
  }
  const int64_t index = in_queue.back();
  in_queue.pop_back();
  const std::string tmp =
      getTmpDir() + "get_program_" + std::to_string(client_id) + ".asm";
  if (!getProgram(index, tmp)) {
    Log::get().debug("Invalid session, resetting.");
    session_id = 0;  // resetting session
    return program;
  }
  Parser parser;
  try {
    program = parser.parse(tmp);
  } catch (const std::exception&) {
    program.ops.clear();
  }
  std::remove(tmp.c_str());
  if (program.ops.empty()) {
    Log::get().warn("Invalid program on API server: " + base_url + "programs/" +
                    std::to_string(index));
  }
  return program;
}

void validateNewSessionIdAndCount(int64_t new_session_id, int64_t new_count) {
  if (new_session_id <= 0) {
    Log::get().error("Received invalid session ID from API server: " +
                         std::to_string(new_session_id),
                     true);
  }
  if (new_count < 0 || new_count > 100000) {  // magic number
    Log::get().error("Received invalid submission count from API server" +
                         std::to_string(new_count),
                     true);
  }
}

int64_t getNumber(const jute::jValue& json, const std::string& name) {
  auto val = const_cast<jute::jValue&>(json)[name];
  if (val.get_type() != jute::JNUMBER) {
    throw std::runtime_error("Invalid JSON response: invalid " + name +
                             " value");
  }
  return val.as_int();
}

void ApiClient::updateSession() {
  Log::get().debug("Updating API client session");
  auto new_session_id = fetchInt("session");
  auto new_count = fetchInt("count");
  validateNewSessionIdAndCount(new_session_id, new_count);
  start = (new_session_id == session_id) ? count : 0;
  count = new_count;
  session_id = new_session_id;
  auto delta_count = count - start;
  in_queue.resize(delta_count);
  for (int64_t i = 0; i < delta_count; i++) {
    in_queue[i] = start + i;
  }
  std::shuffle(in_queue.begin(), in_queue.end(), Random::get().gen);
}

void ApiClient::updateSessionV2() {
  Log::get().info("Updating API client session using v2 API");
  auto json = WebClient::getJson(base_url_v2 + "submissions?limit=1");
  auto new_session_id = getNumber(json, "session");
  auto new_count = getNumber(json, "total");
  validateNewSessionIdAndCount(new_session_id, new_count);
  start = (new_session_id == session_id) ? count : 0;
  count = new_count;
  session_id = new_session_id;

  // Update pages for fetching submissions
  static constexpr int64_t PAGE_SIZE = 100;
  int64_t num_pages = (count / PAGE_SIZE) + 1;
  pages.clear();
  for (int64_t i = 0; i < num_pages; i++) {
    Page page;
    page.start = i * PAGE_SIZE;
    page.count = std::min<int64_t>(PAGE_SIZE, count - page.start);
    pages.push_back(page);
  }
  std::shuffle(pages.begin(), pages.end(), Random::get().gen);
}

int64_t ApiClient::fetchInt(const std::string& endpoint) {
  const std::string tmp =
      getTmpDir() + "tmp_int_" + std::to_string(client_id) + ".txt";
  WebClient::get(base_url + endpoint, tmp, true, true);
  std::ifstream in(tmp);
  if (in.bad()) {
    Log::get().error("Error fetching data from API server", true);
  }
  int64_t value = 0;
  in >> value;
  in.close();
  std::remove(tmp.c_str());
  return value;
}
