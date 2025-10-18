#pragma once

#include <string>

#include "form/expression.hpp"
#include "form/formula.hpp"

/**
 * Parser for Formula objects. Parses formulas from their string representation
 * as produced by Formula::toString().
 *
 * Example input: "a(n) = a(n-1)+a(n-2), a(1) = 1, a(0) = 0"
 */
class FormulaParser {
 public:
  /**
   * Parse a formula from its string representation.
   * @param str The formula string to parse
   * @param formula The output formula object
   * @return true if parsing was successful, false otherwise
   */
  bool parse(const std::string& str, Formula& formula);

 private:
  std::string input;
  size_t pos;

  void skipWhitespace();
  char peek();
  char next();
  bool match(char c);
  bool match(const std::string& s);
  
  Expression parseExpression();
  Expression parseComparison();
  Expression parseAddSub();
  Expression parseTerm();
  Expression parsePower();
  Expression parseUnary();
  Expression parsePrimary();
  Expression parseFunction();
  std::string parseName();
  Number parseNumber();
};
