#pragma once

#include <chrono>

#include "lang/program.hpp"
#include "mine/submission.hpp"
#include "sys/jute.h"

class ApiClient {
 public:
  ApiClient();

  static ApiClient& getDefaultInstance();

  std::string serialize(std::string);

  std::string toJson(const Program& program);
  
  void postProgram(const Program& program, size_t max_buffer = 0);

  bool postProgram(const std::string& path, bool fail_on_error = true);
  
  bool postSubmission(const std::string& path, bool fail_on_error = true);

  void postCPUHour();

  void getOeisFile(const std::string& filename, const std::string& local_path);

  Submission getNextSubmission();

 private:
  // throttle download of OEIS file from API server
  static constexpr int64_t OEIS_THROTTLING_SECS = 10;  // magic number

  class Page {
   public:
    int64_t limit;
    int64_t skip;
  };

  std::string base_url;
  std::string base_url_v2;
  bool oeis_fetch_direct;
  int64_t client_id;
  int64_t session_id;
  int64_t start;
  int64_t count;
  int64_t fetched_oeis_files;
  std::vector<Page> pages;
  std::vector<Submission> in_queue;
  std::vector<Program> out_queue;
  std::chrono::time_point<std::chrono::steady_clock> last_oeis_time;
  bool printed_throttling_warning;

  void updateSession();

  jute::jValue getSubmissions(const Page& page, Submission::Type type);
};
