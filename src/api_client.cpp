#include "api_client.hpp"

#include <fstream>

#include "file.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "setup.hpp"
#include "util.hpp"
#include "web_client.hpp"

const std::string ApiClient::BASE_URL("http://api.loda-lang.org/miner/v1/");

ApiClient::ApiClient()
    : client_id(Random::get().gen() % 100000),
      session_id(0),
      start(0),
      count(0) {}

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
  const std::string url = BASE_URL + "programs";
  if (!WebClient::postFile(url, path)) {
    const std::string msg("Cannot submit program to API server");
    if (fail_on_error) {
      if (!WebClient::postFile(url, path, "", true)) {
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
  const std::string url = BASE_URL + "cpuhours";
  if (!WebClient::postFile(url, {})) {
    Log::get().warn("Error reporting CPU hour");
  }
}

bool ApiClient::getOeisFile(const std::string& filename,
                            const std::string& local_path) {
  const std::string url = BASE_URL + "oeis/" + filename + ".gz";
  const bool success = WebClient::get(url, local_path + ".gz", false, false);
  if (success) {
    gunzip(local_path + ".gz");
  }
  return success;
}

bool ApiClient::getProgram(int64_t index, const std::string& path) {
  std::remove(path.c_str());
  return WebClient::get(BASE_URL + "programs/" + std::to_string(index), path,
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
    Log::get().warn("Invalid program on API server: " + BASE_URL + "programs/" +
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
  WebClient::get(BASE_URL + endpoint, tmp, true, true);
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
