#include "mine/api_client.hpp"

#include <fstream>
#include <thread>

#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "sys/file.hpp"
#include "sys/git.hpp"
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
  const std::string url = base_url + "cpuhours";
  if (!WebClient::postFile(url, {}) &&
      !WebClient::postFile(url, {}, {}, {}, true)) {
    Log::get().warn("Cannot report CPU hour");
  }
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
  if (oeis_fetch_direct) {
    url = "https://www.oeis.org/";
    if (filename.front() == 'b') {
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
      Git::gunzip(local_path + ".gz");
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
