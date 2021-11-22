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

void ApiClient::postProgram(const Program& program) {
  const std::string tmp =
      getTmpDir() + "post_program_" + std::to_string(client_id) + ".asm";
  std::ofstream out(tmp);
  ProgramUtil::print(program, out);
  out.close();
  postProgram(tmp);
  std::remove(tmp.c_str());
}

void ApiClient::postProgram(const std::string& path) {
  if (!isFile(path)) {
    Log::get().error("File not found: " + path, true);
  }
  if (!WebClient::postFile(BASE_URL + "programs", path)) {
    Log::get().error("Error submitting program to API server", true);
  }
}

bool ApiClient::getProgram(int64_t index, const std::string& path) {
  std::remove(path.c_str());
  return WebClient::get(BASE_URL + "programs/" + std::to_string(index), path,
                        false, false);
}

Program ApiClient::getNextProgram() {
  if (session_id == 0 || queue.empty()) {
    updateSession();
  }
  Program program;
  if (queue.empty()) {
    return program;
  }
  const int64_t index = queue.back();
  queue.pop_back();
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
  queue.resize(delta_count);
  for (int64_t i = 0; i < delta_count; i++) {
    queue[i] = start + i;
  }
  // Log::get().debug("updated session:" + std::to_string(session_id) + ",
  // updated start:" + std::to_string(start) + ", updated count:"
  // +std::to_string(count) + " queue: " + std::to_string(queue.size()));
  std::shuffle(queue.begin(), queue.end(), Random::get().gen);
}

int64_t ApiClient::fetchInt(const std::string& endpoint) {
  // TODO: store in tmp folder
  const std::string tmp = "tmp_int.txt";
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
