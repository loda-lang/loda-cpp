#pragma once

#include <chrono>

#include "lang/program.hpp"
#include "sys/jute.h"

class Submission {
 public:
  std::string id;
  std::string submitter;
  std::string type;
  std::string mode;
  Program program;
};

class ApiClient {
 public:
  ApiClient();

  static ApiClient& getDefaultInstance();

  void postProgram(const Program& program, size_t max_buffer = 0);

  bool postProgram(const std::string& path, bool fail_on_error = true);

  void postCPUHour();

  void getOeisFile(const std::string& filename, const std::string& local_path);

  Submission getNextSubmission();

  Program getNextProgram();

 private:
  // throttle download of OEIS file from API server
  static constexpr int64_t OEIS_THROTTLING_SECS = 10;  // magic number
  static constexpr int64_t V2_SUBMISSIONS_PAGE_SIZE =
      100;  // page size for v2 API

  std::string base_url;
  std::string base_url_v2;
  bool oeis_fetch_direct;
  bool use_v2_api;
  int64_t client_id;
  int64_t session_id;
  int64_t start;
  int64_t count;
  int64_t fetched_oeis_files;
  std::vector<int64_t> in_queue;        // for v1 API (program indices)
  std::vector<Submission> v2_in_queue;  // for v2 API (full programs)
  std::vector<Program> out_queue;
  std::chrono::time_point<std::chrono::steady_clock> last_oeis_time;
  bool printed_throttling_warning;

  bool getProgram(int64_t index, const std::string& path);
  bool getSubmission(int64_t index, const std::string& path);

  void updateSession();

  void updateSessionV2();

  int64_t fetchInt(const std::string& endpoint);

  jute::jValue fetchJsonV2(const std::string& endpoint);

  void extractAndUpdateSession(const jute::jValue& json);
};
