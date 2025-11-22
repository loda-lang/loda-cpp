#include "mine/submission.hpp"

#include <sstream>
#include <stdexcept>

#include "lang/parser.hpp"

std::string getStringField(const jute::jValue& json, const std::string& name,
                           bool required = true) {
  auto field = const_cast<jute::jValue&>(json)[name];
  if (field.get_type() == jute::JSTRING) {
    return field.as_string();
  } else if (required) {
    throw std::runtime_error("Missing or invalid '" + name +
                             "' field in submission JSON");
  }
  return "";
}

Submission Submission::fromJson(const jute::jValue& json) {
  Submission submission;
  submission.id = UID(getStringField(json, "id"));
  submission.type = typeFromString(getStringField(json, "type"));
  submission.mode = modeFromString(getStringField(json, "mode"));
  submission.content = getStringField(json, "content");
  submission.submitter = getStringField(json, "submitter", false);
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
