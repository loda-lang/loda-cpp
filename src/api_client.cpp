#include "api_client.hpp"

#include <fstream>

#include "file.hpp"
#include "parser.hpp"
#include "util.hpp"

const std::string ApiClient::BASE_URL("http://api.loda-lang.org/miner/v1/");

ApiClient::ApiClient() : session_id(0) {}

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
  return Http::get(BASE_URL + "programs/" + std::to_string(index), path, false);
}

Program ApiClient::getNextProgram() {
  if (session_id == 0 ||
      (queue.empty() && Random::get().gen() % 10 == 0)) {  // magic number
    resetSession();
  }
  Program program;
  if (!queue.empty()) {
    const int64_t index = queue.back();
    queue.pop_back();
    // TODO: store in tmp folder
    const std::string tmp = "tmp_program.asm";
    if (!getProgram(index, tmp)) {
      resetSession();
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
      Log::get().warn("Invalid program on API server: " + BASE_URL +
                      "programs/" + std::to_string(index));
    }
  }
  return program;
}

void ApiClient::resetSession() {
  session_id = fetchInt("session");
  if (session_id == 0) {
    Log::get().error("Received invalid session ID from API server: " +
                         std::to_string(session_id),
                     true);
  }
  int64_t count = fetchInt("count");
  if (count < 0 || count > 100000) {  // magic number
    Log::get().error("Received invalid program count from API server" +
                         std::to_string(count),
                     true);
  }
  queue.resize(count);
  for (int64_t i = 0; i < count; i++) {
    queue[i] = i;
  }
  std::shuffle(queue.begin(), queue.end(), Random::get().gen);
}

int64_t ApiClient::fetchInt(const std::string& endpoint) {
  // TODO: store in tmp folder
  const std::string tmp = "tmp_int.txt";
  Http::get(BASE_URL + endpoint, tmp, true);
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
