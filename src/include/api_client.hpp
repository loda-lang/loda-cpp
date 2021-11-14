#pragma once

#include "program.hpp"

class ApiClient {
 public:
  static const std::string BASE_URL;

  ApiClient();

  void postProgram(const Program& program);

  void postProgram(const std::string& path);

  Program getNextProgram();

 private:
  int64_t client_id;
  int64_t session_id;
  int64_t start;
  int64_t count;
  std::string local_programs_path;
  std::vector<int64_t> queue;

  bool getProgram(int64_t index, const std::string& path);

  void updateSession();

  int64_t fetchInt(const std::string& endpoint);
};
