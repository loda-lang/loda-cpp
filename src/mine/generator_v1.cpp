#include "mine/generator_v1.hpp"

#include <ctype.h>

#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>

#include "lang/interpreter.hpp"
#include "lang/number.hpp"
#include "lang/optimizer.hpp"
#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "lang/semantics.hpp"
#include "mine/distribution.hpp"
#include "mine/stats.hpp"
#include "sys/log.hpp"

#define POSITION_RANGE 100

GeneratorV1::GeneratorV1(const Config &config, const Stats &stats)
    : Generator(config, stats), current_template(0), mutator(stats) {
  // the post processing adds operations, so we reduce the target length here
  num_operations = std::max<int64_t>(config.length / 2, 1);

  std::string operation_types =
      "^";  // negate operation types (exclusion pattern)
  if (!config.loops) {
    operation_types += "l";
  }
  if (!config.calls) {
    operation_types += "q";
  }
  std::string operand_types = config.indirect_access ? "cdi" : "cd";

  // parse operation types
  bool negate = false;
  for (char c : operation_types) {
    c = tolower(c);
    if (c == '^') {
      negate = true;
    } else {
      auto type = Operation::Type::NOP;
      for (auto &t : Operation::Types) {
        const auto &m = Operation::Metadata::get(t);
        if (m.is_public && m.short_name == c) {
          type = t;
          break;
        }
      }
      if (type == Operation::Type::NOP) {
        Log::get().error("Unknown operation type: " + std::string(1, c), true);
      }
      if (type != Operation::Type::LPE) {
        this->operation_types.push_back(type);
      }
    }
  }
  if (negate) {
    std::vector<Operation::Type> tmp_types;
    for (auto t : Operation::Types) {
      bool found = false;
      for (auto o : this->operation_types) {
        if (o == t) {
          found = true;
          break;
        }
      }
      if (!found && Operation::Metadata::get(t).is_public &&
          t != Operation::Type::LPE) {
        tmp_types.push_back(t);
      }
    }
    this->operation_types = tmp_types;
  }
  if (operation_types.empty()) {
    Log::get().error("No operation types", true);
  }

  std::vector<double> source_type_rates;
  std::vector<double> target_type_rates;
  if (operand_types.find('c') != std::string::npos) {
    source_operand_types.push_back(Operand::Type::CONSTANT);
    source_type_rates.push_back(4);
  }
  if (operand_types.find('d') != std::string::npos) {
    source_operand_types.push_back(Operand::Type::DIRECT);
    source_type_rates.push_back(4);
    target_operand_types.push_back(Operand::Type::DIRECT);
    target_type_rates.push_back(4);
  }
  if (operand_types.find('i') != std::string::npos) {
    source_operand_types.push_back(Operand::Type::INDIRECT);
    source_type_rates.push_back(1);
    target_operand_types.push_back(Operand::Type::INDIRECT);
    target_type_rates.push_back(1);
  }
  if (source_operand_types.empty()) {
    Log::get().error("No source operation types", true);
  }
  if (target_operand_types.empty()) {
    Log::get().error("No target operation types", true);
  }

  // load program templates
  Parser parser;
  Program p;
  for (const auto &t : config.templates) {
    try {
      p = parser.parse(t);
      ProgramUtil::removeOps(p, Operation::Type::NOP);
      for (auto &op : p.ops) {
        op.comment.clear();
      }
      templates.push_back(p);
    } catch (const std::exception &) {
      Log::get().warn("Cannot load template (ignoring): " + t);
    }
  }

  // initialize distributions
  constants.resize(stats.num_constants.size());
  size_t i = 0;
  for (const auto &c : stats.num_constants) {
    constants[i++] = c.first;
  }

  constants_dist = constantsDist(constants, stats);
  operation_dist = operationDist(stats, this->operation_types);
  target_type_dist = std::discrete_distribution<>(target_type_rates.begin(),
                                                  target_type_rates.end());
  target_value_dist = uniformDist(config.max_constant + 1);
  source_type_dist = std::discrete_distribution<>(source_type_rates.begin(),
                                                  source_type_rates.end());
  source_value_dist = uniformDist(config.max_constant + 1);
  position_dist = uniformDist(POSITION_RANGE);
}

std::pair<Operation, double> GeneratorV1::generateOperation() {
  Operation op;
  op.type = operation_types.at(operation_dist(Random::get().gen));
  op.target.type = target_operand_types.at(target_type_dist(Random::get().gen));
  op.target.value = target_value_dist(Random::get().gen);
  op.source.type = source_operand_types.at(source_type_dist(Random::get().gen));
  op.source.value = source_value_dist(Random::get().gen);

  // check number of operands
  if (Operation::Metadata::get(op.type).num_operands < 2) {
    op.source.type = Operand::Type::CONSTANT;
    op.source.value = 0;
  }
  if (Operation::Metadata::get(op.type).num_operands < 1) {
    op.target.type = Operand::Type::CONSTANT;
    op.target.value = 0;
  }

  // bias for constant loop fragment length
  if (op.type == Operation::Type::LPB &&
      op.source.type != Operand::Type::CONSTANT &&
      position_dist(Random::get().gen) % 10 > 0) {
    op.source.type = Operand::Type::CONSTANT;
  }

  // use constants distribution from stats
  if (op.source.type == Operand::Type::CONSTANT) {
    op.source.value = constants.at(constants_dist(Random::get().gen));
    if (op.type == Operation::Type::LPB || op.type == Operation::Type::CLR ||
        op.type == Operation::Type::SRT) {
      op.source.value = Semantics::mod(
          Semantics::max(op.source.value, Number::ONE), 10);  // magic number
    }
  }

  // avoid meaningless zeros or singularities
  ProgramUtil::avoidNopOrOverflow(op);

  std::pair<Operation, double> next_op;
  next_op.first = op;
  next_op.second =
      static_cast<double>(position_dist(Random::get().gen)) / POSITION_RANGE;
  return next_op;
}

Program GeneratorV1::generateProgram() {
  // use template for base program
  Program p;
  if (!templates.empty()) {
    p = templates[current_template];
    current_template = (current_template + 1) % templates.size();
  }
  if (p.ops.empty() || (Random::get().gen() % 2)) {
    generateStateless(p, num_operations);
    applyPostprocessing(p);
  } else {
    mutator.mutateRandom(p);
  }
  return p;
}

bool GeneratorV1::supportsRestart() const { return true; }

bool GeneratorV1::isFinished() const { return false; };
