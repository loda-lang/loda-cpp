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
  int64_t session_id;
  std::vector<int64_t> queue;

  bool getProgram(int64_t index, const std::string& path);

  void resetSession();

  int64_t fetchInt(const std::string& endpoint);
};
