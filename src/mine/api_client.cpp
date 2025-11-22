#include "mine/api_client.hpp"

#include <fstream>
#include <sstream>
#include <thread>

#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "sys/file.hpp"
#include "sys/git.hpp"
#include "sys/jute.h"
#include "sys/log.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"
#include "sys/web_client.hpp"

ApiClient::ApiClient()
    : use_v2_api(true),
      client_id(Random::get().gen() % 100000),
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
  return WebClient::get(base_url + "programs/" + std::to_string(index),
                        path, false, false);
}

bool ApiClient::getSubmission(int64_t index, const std::string &path) {
  std::remove(path.c_str());
  return WebClient::get(
      base_url_v2 + "submissions?skip=" + std::to_string(index) + "&limit=1",
      path, false, false);
}

Submission ApiClient::getNextSubmission() {
  if (session_id == 0 || in_queue.empty()) {
    updateSession();
  }
  Submission submission;
  if (in_queue.empty()) {
    return submission;
  }
  const int64_t index = in_queue.back();
  in_queue.pop_back();
  const std::string tmp =
      getTmpDir() + "get_submission_" + std::to_string(client_id) + ".json";
  if (!getSubmission(index, tmp)) {
    Log::get().debug("Invalid session, resetting.");
    session_id = 0; // resetting session
    return submission;
  }
  jute::parser json_parser;
  try {
    auto tmp2 = json_parser.parse_file(tmp)["results"][0];
    submission.id = tmp2["id"].as_string();
    submission.submitter = tmp2["submitter"].as_string();
    submission.type = tmp2["type"].as_string();
    submission.mode = tmp2["mode"].as_string();
    std::istringstream parse_stream(tmp2["content"].as_string());
    Parser program_parser;
    try {
      submission.program = program_parser.parse(parse_stream);
    } catch (const std::exception &) {
      submission.program.ops.clear();
    }
    std::remove(tmp.c_str());
  } catch (const std::exception &) {
    Log::get().error("Error deserializing next submission from API server",
                     true);
  }
  return submission;
}

Program ApiClient::getNextProgram() {
  if (use_v2_api) {
    if (v2_in_queue.empty()) {
      try {
        updateSessionV2();
      } catch (const std::exception& e) {
        Log::get().warn("Error using v2 API, falling back to v1: " +
                        std::string(e.what()));
        use_v2_api = false;
        session_id = 0;  // reset v1 session
      }
    }

    // Return program from v2 queue if available
    if (!v2_in_queue.empty()) {
      Program program = v2_in_queue.back();
      v2_in_queue.pop_back();
      return program;
    }
  }

  if (!use_v2_api) {
    if (session_id == 0 || in_queue.empty()) {
      updateSession();
    }
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
    Log::get().warn("Invalid program on API server: " + base_url +
                    "programs/" + std::to_string(index));
  }
  return program;
}

void ApiClient::updateSession() {
  Log::get().debug("Updating API client session");
  auto new_session_id = fetchInt("session");
  if (new_session_id == 0) {
    Log::get().error("Received invalid session ID from API server: " +
                         std::to_string(new_session_id),
                     true);
  }
  auto new_count = fetchInt("count");
  if (new_count < 0 || new_count > 100000) {  // magic number
    Log::get().error("Received invalid program count from API server" +
                         std::to_string(new_count),
                     true);
  }
  // Log::get().debug("old session:" + std::to_string(session_id) + ", old
  // start:" + std::to_string(start) + ", old count:" +std::to_string(count) );
  // Log::get().debug("new session:" + std::to_string(new_session_id) + ", new
  // count:" +std::to_string(new_count) );
  start = (new_session_id == session_id) ? count : 0;
  count = new_count;
  session_id = new_session_id;
  auto delta_count = count - start;
  in_queue.resize(delta_count);
  for (int64_t i = 0; i < delta_count; i++) {
    in_queue[i] = start + i;
  }
  // Log::get().debug("updated session:" + std::to_string(session_id) + ",
  // updated start:" + std::to_string(start) + ", updated count:"
  // +std::to_string(count) + " queue: " + std::to_string(queue.size()));
  std::shuffle(in_queue.begin(), in_queue.end(), Random::get().gen);
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

jute::jValue ApiClient::fetchJsonV2(const std::string& endpoint) {
  const std::string tmp =
      getTmpDir() + "submissions_" + std::to_string(client_id) + ".json";

  if (!WebClient::get(base_url_v2 + endpoint, tmp, true, false)) {
    std::remove(tmp.c_str());
    throw std::runtime_error("Failed to fetch from v2 API: " + endpoint);
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

  if (json.get_type() != jute::JOBJECT) {
    throw std::runtime_error("Invalid JSON response: expected object");
  }

  return json;
}

void ApiClient::extractAndUpdateSession(const jute::jValue& json) {
  // Extract session ID
  auto session_val = const_cast<jute::jValue&>(json)["session"];
  if (session_val.get_type() == jute::JNUMBER) {
    int64_t new_session_id = static_cast<int64_t>(session_val.as_double());
    if (new_session_id != session_id) {
      Log::get().debug("Session changed from " + std::to_string(session_id) +
                       " to " + std::to_string(new_session_id));
      session_id = new_session_id;
    }
  }

  // Extract and update count
  auto total_val = const_cast<jute::jValue&>(json)["total"];
  if (total_val.get_type() == jute::JNUMBER) {
    int64_t new_count = static_cast<int64_t>(total_val.as_double());
    if (new_count != count) {
      Log::get().debug("Count changed from " + std::to_string(count) + " to " +
                       std::to_string(new_count));
      count = new_count;
    }
  }
}

void ApiClient::updateSessionV2() {
  Log::get().debug("Updating API client session using v2 API");

  // Check if we need to fetch session and count
  if (session_id == 0) {
    // First time or session expired - fetch to get session and total count
    jute::jValue count_json = fetchJsonV2("submissions?mode=next&limit=1");
    extractAndUpdateSession(count_json);
  }

  Log::get().debug("Session: " + std::to_string(session_id) +
                   ", total submissions: " + std::to_string(count));

  if (count <= 0) {
    v2_in_queue.clear();
    return;
  }

  // Calculate how many programs to fetch (at most V2_SUBMISSIONS_PAGE_SIZE)
  int64_t num_to_fetch = std::min<int64_t>(V2_SUBMISSIONS_PAGE_SIZE, count);

  // Generate random skip offset within the valid range
  int64_t max_skip = std::max<int64_t>(0, count - num_to_fetch);
  int64_t random_skip =
      max_skip > 0 ? (Random::get().gen() % (max_skip + 1)) : 0;

  // Fetch submissions at random offset
  std::stringstream endpoint;
  endpoint << "submissions?type=program&limit=" << num_to_fetch
           << "&skip=" << random_skip;

  jute::jValue json = fetchJsonV2(endpoint.str());

  // Update session ID and count from the actual fetch response
  extractAndUpdateSession(json);

  auto submissions = json["submissions"];
  if (submissions.get_type() != jute::JARRAY) {
    throw std::runtime_error(
        "Invalid JSON response: missing submissions array");
  }

  // Extract and parse programs from submissions
  v2_in_queue.clear();
  Parser parser;

  for (int i = 0; i < submissions.size(); i++) {
    auto submission = submissions[i];

    // Check if this is a program submission (not sequence)
    auto obj_type = submission["type"];
    if (obj_type.get_type() == jute::JSTRING &&
        obj_type.as_string() != "program") {
      continue;  // Skip non-program submissions
    }

    // Extract the program code
    auto code_val = submission["code"];
    if (code_val.get_type() != jute::JSTRING) {
      continue;  // Skip if no code field
    }

    std::string code = code_val.as_string();
    if (code.empty()) {
      continue;
    }

    // Parse the program code
    try {
      std::stringstream code_stream(code);
      Program program = parser.parse(code_stream);
      if (!program.ops.empty()) {
        v2_in_queue.push_back(program);
      }
    } catch (const std::exception& e) {
      // Extract ID for logging
      std::string id_str = "unknown";
      auto id_val = submission["id"];
      if (id_val.get_type() == jute::JSTRING) {
        id_str = id_val.as_string();
      }
      Log::get().warn("Failed to parse program " + id_str + ": " +
                      std::string(e.what()));
    }
  }

  // Shuffle the queue for additional randomization
  std::shuffle(v2_in_queue.begin(), v2_in_queue.end(), Random::get().gen);

  Log::get().debug("Fetched " + std::to_string(v2_in_queue.size()) +
                   " programs from v2 API at random offset " +
                   std::to_string(random_skip));
}
