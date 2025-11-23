#pragma once

#include <string>

#include "base/uid.hpp"
#include "lang/program.hpp"
#include "sys/jute.h"

class Submission {
 public:
  enum class Type { PROGRAM, SEQUENCE };
  enum class Mode { ADD, UPDATE, REMOVE };

  UID id;
  Type type;
  Mode mode;
  std::string content;
  std::string submitter;

  static Submission fromJson(const jute::jValue& json);

  Program toProgram() const;

  static Mode modeFromString(const std::string& mode_str);
  static Type typeFromString(const std::string& type_str);
  static std::string modeToString(Mode mode);
  static std::string typeToString(Type type);
};
