#pragma once

#include <memory>

#include "lang/program.hpp"

class Parser {
 public:
  Parser() : in(nullptr) {}

  Program parse(const std::string &file);

  Program parse(std::istream &in);

  void readSeparator(char separator);

  Number readValue();

  Number readNonNegativeValue();

  std::string readIdentifier();

  Operand readOperand();

  void readOperationType(Operation &op);

  std::istream *in;
  std::shared_ptr<std::ifstream> file_in;
};
