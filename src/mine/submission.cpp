#include "mine/submission.hpp"

#include <sstream>
#include <stdexcept>

#include "lang/parser.hpp"

Submission Submission::fromJson(const jute::jValue& json) {
  Submission submission;
  auto id = const_cast<jute::jValue&>(json)["id"];
  if (id.get_type() == jute::JSTRING) {
    submission.id = UID(id.as_string());
  }
  auto type_ = const_cast<jute::jValue&>(json)["type"];
  if (type_.get_type() == jute::JSTRING) {
    submission.type = typeFromString(type_.as_string());
  } else {
    throw std::runtime_error(
        "Missing or invalid 'type' field in submission JSON");
  }
  auto mode = const_cast<jute::jValue&>(json)["mode"];
  if (mode.get_type() == jute::JSTRING) {
    submission.mode = modeFromString(mode.as_string());
  } else {
    throw std::runtime_error(
        "Missing or invalid 'mode' field in submission JSON");
  }
  auto content = const_cast<jute::jValue&>(json)["content"];
  if (content.get_type() == jute::JSTRING) {
    submission.content = content.as_string();
  } else {
    throw std::runtime_error(
        "Missing or invalid 'content' field in submission JSON");
  }
  auto submitter = const_cast<jute::jValue&>(json)["submitter"];
  if (submitter.get_type() == jute::JSTRING) {
    submission.submitter = submitter.as_string();
  }
  return submission;
}

Program Submission::toProgram() const {
  Program program;
  if (content.empty()) {
    return program;
  }
  Parser parser;
  std::istringstream parse_stream(content);
  return parser.parse(parse_stream);
}

Submission::Mode Submission::modeFromString(const std::string& mode_str) {
  if (mode_str == "add") {
    return Mode::ADD;
  } else if (mode_str == "update") {
    return Mode::UPDATE;
  } else if (mode_str == "delete") {
    return Mode::DELETE;
  } else {
    throw std::runtime_error("Invalid submission mode: " + mode_str);
  }
}

Submission::Type Submission::typeFromString(const std::string& type_str) {
  if (type_str == "program") {
    return Type::PROGRAM;
  } else if (type_str == "sequence") {
    return Type::SEQUENCE;
  } else {
    throw std::runtime_error("Invalid submission type: " + type_str);
  }
}

std::string Submission::modeToString(Mode mode) {
  switch (mode) {
    case Mode::ADD:
      return "add";
    case Mode::UPDATE:
      return "update";
    case Mode::DELETE:
      return "delete";
  }
  return "unknown";
}

std::string Submission::typeToString(Type type) {
  switch (type) {
    case Type::PROGRAM:
      return "program";
    case Type::SEQUENCE:
      return "sequence";
  }
  return "unknown";
}
