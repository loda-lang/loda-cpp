#include "api_client.hpp"

#include <fstream>

#include "file.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "setup.hpp"
#include "util.hpp"

const std::string ApiClient::BASE_URL("http://api.loda-lang.org/miner/v1/");

ApiClient::ApiClient() : session_id(0), start(0), count(0) {}

void ApiClient::postProgram(const Program& program) {
  // TODO: store in tmp folder
  const std::string tmp = "tmp_program.asm";
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
  if (!Http::postFile(BASE_URL + "programs", path)) {
    Log::get().error("Error submitting program to miner API", true);
  }
}

bool ApiClient::getProgram(int64_t index, const std::string& path) {
  std::remove(path.c_str());
  return Http::get(BASE_URL + "programs/" + std::to_string(index), path, false,
                   false);
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
  if (local_programs_path.empty()) {
    local_programs_path = Setup::getProgramsHome() + "local/";
    ensureDir(local_programs_path);
  }
  const std::string tmp =
      local_programs_path + "api-" + std::to_string(index) + ".asm";
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
  // std::remove(tmp.c_str());
  if (program.ops.empty()) {
    Log::get().warn("Invalid program on API server: " + BASE_URL + "programs/" +
                    std::to_string(index));
  }
  return program;
}

void ApiClient::updateSession() {
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
  start = (new_session_id == session_id) ? count : 0;
  count = new_count - start;
  session_id = new_session_id;
  queue.resize(count);
  for (int64_t i = start; i < count; i++) {
    queue[i] = i;
  }
  std::shuffle(queue.begin(), queue.end(), Random::get().gen);
}

int64_t ApiClient::fetchInt(const std::string& endpoint) {
  // TODO: store in tmp folder
  const std::string tmp = "tmp_int.txt";
  Http::get(BASE_URL + endpoint, tmp, true, true);
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
