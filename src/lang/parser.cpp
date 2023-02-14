#include "lang/parser.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "lang/number.hpp"
#include "lang/program.hpp"
#include "sys/util.hpp"

Program Parser::parse(const std::string &file) {
  file_in.reset(new std::ifstream(file));
  if (!file_in->good()) {
    throw std::runtime_error("Error opening file: " + file);
  }
  auto p = parse(*file_in);
  file_in->close();
  return p;
}

Program Parser::parse(std::istream &in_) {
  in = &in_;
  Program p;
  Operation o;
  std::string l;
  while (true) {
    *in >> std::ws;
    auto c = in->peek();
    if (c == EOF) {
      break;
    }
    o = Operation();
    if (c != ';') {
      // read normal operation
      o.type = readOperationType();
      switch (Operation::Metadata::get(o.type).num_operands) {
        case 0:
          o.target = Operand(Operand::Type::CONSTANT, Number::ZERO);
          o.source = Operand(Operand::Type::CONSTANT, Number::ZERO);
          break;
        case 1:
          o.target = readOperand();
          o.source = Operand(Operand::Type::CONSTANT, Number::ZERO);
          break;
        case 2:
          o.target = readOperand();
          if (o.type ==
              Operation::Type::LPB)  // lpb has an optional second argument
          {
            c = in->peek();
            while (c == ' ' || c == '\t') {
              in->get();
              c = in->peek();
            }
            if (c == ',') {
              readSeparator(',');
              o.source = readOperand();
            } else {
              o.source =
                  Operand(Operand::Type::CONSTANT,
                          Number::ONE);  // default second argument is 1 for lpb
            }
          } else {
            readSeparator(',');
            o.source = readOperand();
          }
          break;
        default:
          throw std::runtime_error("invalid number of operands");
      }
    }

    // read comment
    c = in->peek();
    while (c == ' ' || c == '\t') {
      in->get();
      c = in->peek();
    }
    if (c == ';') {
      in->get();
      c = in->peek();
      while (c == ' ' || c == '\t' || c == ';') {
        in->get();
        c = in->peek();
      }
      std::getline(*in, l);
      while (!l.empty() && std::isspace(l.back())) {
        l.pop_back();
      }
      o.comment = l;
    }

    // add operation to program
    if (o.type != Operation::Type::NOP || !o.comment.empty()) {
      p.ops.push_back(o);
    }
  }
  return p;
}

void Parser::readSeparator(char separator) {
  *in >> std::ws;
  if (in->get() != separator) {
    throw std::runtime_error("expected separator");
  }
}

Number Parser::readValue() {
  std::string buf;
  Number::readIntString(*in, buf);
  return Number(buf);
}

Number Parser::readNonNegativeValue() {
  auto value = readValue();
  if (value < Number::ZERO) {
    throw std::runtime_error("negative value not allowed");
  }
  return value;
}

std::string Parser::readIdentifier() {
  int c;
  *in >> std::ws;
  c = in->get();
  if (c == '_' || std::isalpha(c)) {
    std::string s;
    s += (char)c;
    while (true) {
      c = in->peek();
      if (c == '_' || std::isalnum(c)) {
        s += (char)c;
        in->get();
      } else {
        break;
      }
    }
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
  } else {
    throw std::runtime_error("invalid identifier");
  }
}

Operand Parser::readOperand() {
  *in >> std::ws;
  int c = in->peek();
  if (c == '$') {
    in->get();
    c = in->peek();
    if (c == '$') {
      in->get();
      return Operand(Operand::Type::INDIRECT, readNonNegativeValue());
    } else {
      return Operand(Operand::Type::DIRECT, readNonNegativeValue());
    }
  } else {
    return Operand(Operand::Type::CONSTANT, readValue());
  }
}

Operation::Type Parser::readOperationType() {
  return Operation::Metadata::get(readIdentifier()).type;
}
