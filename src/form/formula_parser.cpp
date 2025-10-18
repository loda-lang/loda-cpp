#include "form/formula_parser.hpp"

#include <cctype>
#include <stdexcept>

bool FormulaParser::parse(const std::string& str, Formula& formula) {
  input = str;
  pos = 0;
  formula.clear();

  try {
    // Parse comma-separated list of entries
    while (true) {
      skipWhitespace();
      if (pos >= input.length()) {
        break;
      }

      // Parse left-hand side (function call)
      Expression lhs = parseFunction();

      skipWhitespace();
      if (!match('=')) {
        return false;
      }
      skipWhitespace();

      // Parse right-hand side (expression)
      Expression rhs = parseExpression();

      // Add entry to formula
      formula.entries[lhs] = rhs;

      skipWhitespace();
      if (pos >= input.length()) {
        break;
      }
      if (!match(',')) {
        return false;
      }
    }
    return true;
  } catch (...) {
    return false;
  }
}

void FormulaParser::skipWhitespace() {
  while (pos < input.length() && std::isspace(input[pos])) {
    pos++;
  }
}

char FormulaParser::peek() {
  if (pos >= input.length()) {
    return '\0';
  }
  return input[pos];
}

char FormulaParser::next() {
  if (pos >= input.length()) {
    return '\0';
  }
  return input[pos++];
}

bool FormulaParser::match(char c) {
  skipWhitespace();
  if (peek() == c) {
    pos++;
    return true;
  }
  return false;
}

bool FormulaParser::match(const std::string& s) {
  skipWhitespace();
  if (input.substr(pos, s.length()) == s) {
    pos += s.length();
    return true;
  }
  return false;
}

Expression FormulaParser::parseExpression() {
  return parseComparison();
}

Expression FormulaParser::parseComparison() {
  // Parse comparison operators (lowest precedence in expressions)
  Expression left = parseAddSub();
  
  skipWhitespace();
  if (match("==")) {
    skipWhitespace();
    Expression right = parseAddSub();
    Expression comp(Expression::Type::EQUAL);
    comp.newChild(left);
    comp.newChild(right);
    return comp;
  } else if (match("!=")) {
    skipWhitespace();
    Expression right = parseAddSub();
    Expression comp(Expression::Type::NOT_EQUAL);
    comp.newChild(left);
    comp.newChild(right);
    return comp;
  } else if (match("<=")) {
    skipWhitespace();
    Expression right = parseAddSub();
    Expression comp(Expression::Type::LESS_EQUAL);
    comp.newChild(left);
    comp.newChild(right);
    return comp;
  } else if (match(">=")) {
    skipWhitespace();
    Expression right = parseAddSub();
    Expression comp(Expression::Type::GREATER_EQUAL);
    comp.newChild(left);
    comp.newChild(right);
    return comp;
  }
  
  return left;
}

Expression FormulaParser::parseAddSub() {
  // Parse addition and subtraction
  Expression left = parseTerm();
  
  while (true) {
    skipWhitespace();
    char op = peek();
    if (op == '+' || op == '-') {
      // Check if this is actually a subtraction operator (not a negative number)
      // It's an operator if we just parsed something on the left
      next();
      skipWhitespace();
      Expression right = parseTerm();
      
      if (op == '+') {
        if (left.type == Expression::Type::SUM) {
          left.newChild(right);
        } else {
          Expression sum(Expression::Type::SUM);
          sum.newChild(left);
          sum.newChild(right);
          left = sum;
        }
      } else {
        // Subtraction: convert to addition with negation
        Expression negOne(Expression::Type::CONSTANT, "", Number(-1));
        Expression negRight;
        if (right.type == Expression::Type::CONSTANT) {
          right.value.negate();
          negRight = right;
        } else if (right.type == Expression::Type::PRODUCT) {
          // Flatten: prepend -1 to existing product
          right.children.insert(right.children.begin(), negOne);
          negRight = right;
        } else {
          negRight = Expression(Expression::Type::PRODUCT);
          negRight.newChild(negOne);
          negRight.newChild(right);
        }
        
        if (left.type == Expression::Type::SUM) {
          left.newChild(negRight);
        } else {
          Expression sum(Expression::Type::SUM);
          sum.newChild(left);
          sum.newChild(negRight);
          left = sum;
        }
      }
    } else {
      break;
    }
  }
  
  return left;
}

Expression FormulaParser::parseTerm() {
  // Parse multiplication, division, and modulus
  Expression left = parsePower();
  
  while (true) {
    skipWhitespace();
    char op = peek();
    if (op == '*' || op == '/' || op == '%') {
      next();
      skipWhitespace();
      Expression right = parsePower();
      
      if (op == '*') {
        if (left.type == Expression::Type::PRODUCT) {
          left.newChild(right);
        } else {
          Expression prod(Expression::Type::PRODUCT);
          prod.newChild(left);
          prod.newChild(right);
          left = prod;
        }
      } else if (op == '/') {
        Expression frac(Expression::Type::FRACTION);
        frac.newChild(left);
        frac.newChild(right);
        left = frac;
      } else {  // op == '%'
        Expression mod(Expression::Type::MODULUS);
        mod.newChild(left);
        mod.newChild(right);
        left = mod;
      }
    } else {
      break;
    }
  }
  
  return left;
}

Expression FormulaParser::parsePower() {
  // Parse exponentiation (right-associative)
  Expression left = parseUnary();
  
  skipWhitespace();
  if (peek() == '^') {
    next();
    skipWhitespace();
    Expression right = parsePower();  // Right-associative
    Expression pow(Expression::Type::POWER);
    pow.newChild(left);
    pow.newChild(right);
    return pow;
  }
  
  return left;
}

Expression FormulaParser::parseUnary() {
  // Parse unary minus and factorial
  skipWhitespace();
  if (peek() == '-') {
    next();
    skipWhitespace();
    Expression operand = parseUnary();
    
    // Convert to multiplication by -1
    Expression negOne(Expression::Type::CONSTANT, "", Number(-1));
    if (operand.type == Expression::Type::CONSTANT) {
      operand.value.negate();
      return operand;
    } else if (operand.type == Expression::Type::PRODUCT) {
      // Flatten: prepend -1 to existing product
      operand.children.insert(operand.children.begin(), negOne);
      return operand;
    } else {
      Expression prod(Expression::Type::PRODUCT);
      prod.newChild(negOne);
      prod.newChild(operand);
      return prod;
    }
  }
  
  Expression result = parsePrimary();
  
  // Check for factorial (but not if it's part of !=)
  skipWhitespace();
  if (peek() == '!') {
    // Only treat as factorial if not followed by '='
    bool isFactorial = true;
    if (pos + 1 < input.length() && input[pos + 1] == '=') {
      isFactorial = false;
    }
    if (isFactorial) {
      next();
      Expression fact(Expression::Type::FACTORIAL);
      fact.newChild(result);
      return fact;
    }
  }
  
  return result;
}

Expression FormulaParser::parsePrimary() {
  skipWhitespace();
  
  // Parenthesized expression
  if (peek() == '(') {
    next();
    Expression expr = parseExpression();
    skipWhitespace();
    if (!match(')')) {
      throw std::runtime_error("Expected ')'");
    }
    return expr;
  }
  
  // Number
  if (std::isdigit(peek())) {
    return Expression(Expression::Type::CONSTANT, "", parseNumber());
  }
  
  // Function or variable
  if (std::isalpha(peek()) || peek() == '_') {
    std::string name = parseName();
    
    skipWhitespace();
    if (peek() == '(') {
      // Function call
      next();
      Expression func(Expression::Type::FUNCTION, name);
      
      // Parse arguments
      skipWhitespace();
      if (peek() != ')') {
        while (true) {
          func.newChild(parseExpression());
          skipWhitespace();
          if (peek() == ')') {
            break;
          }
          if (!match(',')) {
            throw std::runtime_error("Expected ',' or ')'");
          }
          skipWhitespace();
        }
      }
      
      if (!match(')')) {
        throw std::runtime_error("Expected ')'");
      }
      
      return func;
    } else {
      // Variable/parameter
      return Expression(Expression::Type::PARAMETER, name);
    }
  }
  
  throw std::runtime_error("Unexpected character");
}

Expression FormulaParser::parseFunction() {
  // Parse function call (for LHS of formula entry)
  std::string name = parseName();
  
  skipWhitespace();
  if (!match('(')) {
    throw std::runtime_error("Expected '('");
  }
  
  Expression func(Expression::Type::FUNCTION, name);
  
  // Parse arguments
  skipWhitespace();
  if (peek() != ')') {
    while (true) {
      func.newChild(parseExpression());
      skipWhitespace();
      if (peek() == ')') {
        break;
      }
      if (!match(',')) {
        throw std::runtime_error("Expected ',' or ')'");
      }
      skipWhitespace();
    }
  }
  
  if (!match(')')) {
    throw std::runtime_error("Expected ')'");
  }
  
  return func;
}

std::string FormulaParser::parseName() {
  std::string name;
  skipWhitespace();
  
  while (pos < input.length() && 
         (std::isalnum(input[pos]) || input[pos] == '_')) {
    name += input[pos];
    pos++;
  }
  
  if (name.empty()) {
    throw std::runtime_error("Expected identifier");
  }
  
  return name;
}

Number FormulaParser::parseNumber() {
  std::string numStr;
  skipWhitespace();
  
  while (pos < input.length() && std::isdigit(input[pos])) {
    numStr += input[pos];
    pos++;
  }
  
  if (numStr.empty()) {
    throw std::runtime_error("Expected number");
  }
  
  return Number(numStr);
}
