#include "mine/api_client.hpp"

#include <fstream>
#include <sstream>
#include <thread>

#include "lang/comments.hpp"
#include "lang/program_util.hpp"
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

std::string ApiClient::serialize(std::string str) {
  std::string result;
  for (char c : str) {
    if (c == '\\') {
      result += "\\\\";
    } else if (c == '\"') {
      result += "\\\"";
    } else if (c == '\n') {
      result += "\\n";
    } else if (c == '\r') {
      // Ignore CR
    } else {
      result += c;
    }
  }
  return result;
}

std::string ApiClient::toJson(const Program& program){
  const std::string id = Comments::getSequenceIdFromProgram(program);
  const std::string submitter = Comments::getSubmitter(program);
  const std::string change_type = Comments::getCommentField(program, Comments::PREFIX_CHANGE_TYPE);
  const std::string mode = (change_type == "" ? "add" : "update");
  const std::string type = "program";
  std::ostringstream oss;
  ProgramUtil::print(program, oss);
  const std::string content = oss.str();
  return "{\"id\":\"" + serialize(id) + "\","
         "\"submitter\":\"" + serialize(submitter) + "\","
         "\"mode\":\"" + serialize(mode) + "\","
         "\"type\":\"" + serialize(type) + "\","
         "\"content\":\"" + serialize(content) + "\"}";
}

void ApiClient::postProgram(const Program& program, size_t max_buffer) {
  // attention: curl sometimes has problems with absolute paths.
  // so we use a relative path here!
  const std::string tmp = "post_program_" + std::to_string(client_id) + ".json";
  out_queue.push_back(program);
  while (!out_queue.empty()) {
    {
      std::ofstream out(tmp);
      out << toJson(out_queue.back());
      out.close();
    }
    if (postSubmission(tmp, out_queue.size() > max_buffer)) {
      out_queue.pop_back();
    } else {
      break;
    }
  }
  std::remove(tmp.c_str());
}

bool ApiClient::postSubmission(const std::string& path, bool fail_on_error) {
  if (!isFile(path)) {
    Log::get().error("File not found: " + path, true);
  }
  const std::string url = base_url_v2 + "submissions";
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

jute::jValue ApiClient::getSubmissions(const Page& page,
                                       Submission::Type type) {
  std::stringstream endpoint;
  endpoint << base_url_v2
           << "submissions?type=" << Submission::typeToString(type);
  if (page.limit != 0) {
    endpoint << "&limit=" << page.limit;
  }
  if (page.skip != 0) {
    endpoint << "&skip=" << page.skip;
  }
  return WebClient::getJson(endpoint.str());
}

Submission ApiClient::getNextSubmission() {
  if (session_id == 0 || pages.empty()) {
    updateSession();
  }
  Submission submission;
  if (pages.empty()) {
    return submission;
  }
  if (in_queue.empty()) {
    // fetch next page
    const Page page = pages.back();
    pages.pop_back();
    auto json = getSubmissions(page, Submission::Type::PROGRAM);
    auto submissions = json["results"];
    if (submissions.get_type() != jute::JARRAY) {
      throw std::runtime_error(
          "Invalid JSON response: missing submissions array");
    }
    in_queue.clear();
    for (int i = 0; i < submissions.size(); i++) {
      // Create submission from JSON (this will validate type and mode)
      Submission sub;
      try {
        sub = Submission::fromJson(submissions[i]);
      } catch (const std::exception& e) {
        Log::get().warn("Failed to parse submission: " + std::string(e.what()));
        continue;
      }
      if (sub.type != Submission::Type::PROGRAM) {
        continue;  // Skip non-program submissions
      }
      // For ADD and UPDATE modes, content is required
      if ((sub.mode == Submission::Mode::ADD ||
           sub.mode == Submission::Mode::UPDATE) &&
          sub.content.empty()) {
        continue;  // Skip if no content for ADD/UPDATE
      }
      // If content is provided, validate that the program can be parsed
      if (!sub.content.empty()) {
        try {
          Program program = sub.toProgram();
          if (program.ops.empty()) {
            continue;  // Skip if program has no operations
          }
        } catch (const std::exception& e) {
          Log::get().warn("Failed to parse program content: " + std::string(e.what()));
          continue;  // Skip if parsing throws
        }
      }
      // Accept the submission (including REMOVE with no content)
      in_queue.push_back(sub);
    }
    std::shuffle(in_queue.begin(), in_queue.end(), Random::get().gen);
  }
  if (in_queue.empty()) {
    return submission;
  }
  submission = in_queue.back();
  in_queue.pop_back();
  return submission;
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
  Page p;
  p.limit = 1;
  p.skip = 0;
  auto json = getSubmissions(p, Submission::Type::PROGRAM);
  auto new_session_id = getNumber(json, "session");
  auto new_count = getNumber(json, "total");
  validateNewSessionIdAndCount(new_session_id, new_count);
  start = (new_session_id == session_id) ? count : 0;
  count = new_count;
  session_id = new_session_id;

  // Update pages for fetching submissions
  static constexpr int64_t PAGE_SIZE = 100;
  int64_t remaining = count - start;
  int64_t num_pages =
      (remaining / PAGE_SIZE) + (remaining % PAGE_SIZE > 0 ? 1 : 0);
  pages.clear();
  for (int64_t i = 0; i < num_pages; i++) {
    Page page;
    page.skip = start + i * PAGE_SIZE;
    page.limit = std::min<int64_t>(PAGE_SIZE, count - page.skip);
    pages.push_back(page);
  }
  std::shuffle(pages.begin(), pages.end(), Random::get().gen);
}
