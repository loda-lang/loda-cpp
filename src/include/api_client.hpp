#pragma once

#include <chrono>

#include "program.hpp"

class ApiClient {
 public:
  static const std::string BASE_URL;

  ApiClient();

  static ApiClient& getDefaultInstance();

  void postProgram(const Program& program, size_t max_buffer = 0);

  bool postProgram(const std::string& path, bool fail_on_error = true);

  void postCPUHour();

  bool getOeisFile(const std::string& filename, const std::string& local_path);

  Program getNextProgram();

 private:
  // throttle download of OEIS file from API server
  static constexpr int64_t OEIS_THROTTLING_SECS = 10;  // magic number

  int64_t client_id;
  int64_t session_id;
  int64_t start;
  int64_t count;
  int64_t fetched_oeis_files;
  std::vector<int64_t> in_queue;
  std::vector<Program> out_queue;
  std::chrono::time_point<std::chrono::steady_clock> last_oeis_time;
  bool printed_throttling_warning;

  bool getProgram(int64_t index, const std::string& path);

  void updateSession();

  int64_t fetchInt(const std::string& endpoint);
};
