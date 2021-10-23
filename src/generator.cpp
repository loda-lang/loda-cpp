#include "generator.hpp"

#include "config.hpp"
#include "generator_v1.hpp"
#include "generator_v2.hpp"
#include "generator_v3.hpp"
#include "generator_v4.hpp"
#include "generator_v5.hpp"
#include "generator_v6.hpp"
#include "semantics.hpp"
#include "util.hpp"

Generator::UPtr Generator::Factory::createGenerator(const Config &config,
                                                    const Stats &stats) {
  Generator::UPtr generator;
  switch (config.version) {
    case 1: {
      generator.reset(new GeneratorV1(config, stats));
      break;
    }
    case 2: {
      generator.reset(new GeneratorV2(config, stats));
      break;
    }
    case 3: {
      generator.reset(new GeneratorV3(config, stats));
      break;
    }
    case 4: {
      generator.reset(new GeneratorV4(config, stats));
      break;
    }
    case 5: {
      generator.reset(new GeneratorV5(config, stats));
      break;
    }
    case 6: {
      generator.reset(new GeneratorV6(config, stats));
      break;
    }
    default: {
      Log::get().error(
          "Unknown generator version: " + std::to_string(config.version), true);
      break;
    }
  }
  return generator;
}

Generator::Generator(const Config &config, const Stats &stats)
    : config(config), found_programs(stats.found_programs) {
  metric_labels = {{"version", std::to_string(config.version)},
                   {"length", std::to_string(config.length)},
                   {"max_constant", std::to_string(config.max_constant)},
                   {"loops", std::to_string(config.loops)},
                   {"indirect", std::to_string(config.indirect_access)}};
  // label values must not be empty
  if (!config.program_template.empty()) {
    auto temp = config.program_template;
    auto n = temp.find_last_of("/");
    if (n != std::string::npos) {
      temp = temp.substr(n + 1);
    }
    metric_labels.emplace("template", temp);
  }
}

void Generator::generateStateless(Program &p, size_t num_operations) {
  // fill program with random operations
  size_t nops = 0;
  while (p.ops.size() + nops < num_operations) {
    auto next_op = generateOperation();
    if (next_op.first.type == Operation::Type::NOP ||
        next_op.first.type == Operation::Type::LPE) {
      nops++;
      continue;
    }
    size_t position = (next_op.second * (p.ops.size() + 1));
    p.ops.emplace(p.ops.begin() + position, Operation(next_op.first));
    if (next_op.first.type == Operation::Type::LPB) {
      position = ((position + p.ops.size()) / 2) + 1;
      p.ops.emplace(p.ops.begin() + position, Operation(Operation::Type::LPE));
    }
  }
}

void Generator::applyPostprocessing(Program &p) {
  auto written_cells = fixCausality(p);
  fixSingularities(p);
  fixCalls(p);
  ensureSourceNotOverwritten(p);
  ensureTargetWritten(p, written_cells);
  ensureMeaningfulLoops(p);
}

std::vector<int64_t> Generator::fixCausality(Program &p) {
  // fix causality of read operations
  std::vector<int64_t> written_cells;
  written_cells.push_back(0);
  int64_t new_cell;
  for (size_t position = 0; position < p.ops.size(); position++) {
    auto &op = p.ops[position];
    auto &meta = Operation::Metadata::get(op.type);

    // fix source operand in new operation
    if (meta.num_operands == 2 && op.source.type == Operand::Type::DIRECT &&
        std::find(written_cells.begin(), written_cells.end(),
                  op.source.value.asInt()) == written_cells.end()) {
      new_cell = op.source.value.asInt() %
                 written_cells.size();  // size of written_cells is >=1
      if (Number(written_cells[new_cell]) == op.target.value) {
        new_cell = (new_cell + 1) % written_cells.size();
      }
      op.source.value = Number(written_cells[new_cell]);
    }

    // fix target operand in new operation
    if (meta.num_operands > 0 && meta.is_reading_target &&
        op.type != Operation::Type::ADD &&
        op.target.type == Operand::Type::DIRECT &&
        std::find(written_cells.begin(), written_cells.end(),
                  op.target.value.asInt()) == written_cells.end()) {
      new_cell = op.target.value.asInt() % written_cells.size();
      if (op.source.type == Operand::Type::DIRECT &&
          Number(written_cells[new_cell]) == op.source.value) {
        new_cell = (new_cell + 1) % written_cells.size();
      }
      op.target.value = Number(written_cells[new_cell]);
    }

    // check if target cell not written yet
    if (meta.is_writing_target && op.target.type == Operand::Type::DIRECT &&
        std::find(written_cells.begin(), written_cells.end(),
                  op.target.value.asInt()) == written_cells.end()) {
      // update written cells
      written_cells.push_back(op.target.value.asInt());
    }
  }
  return written_cells;
}

void Generator::fixSingularities(Program &p) {
  static const Operand tmp(Operand::Type::DIRECT, 26);  // magic number
  static const int64_t max_exponent = 5;                // magic number
  for (size_t i = 0; i < p.ops.size(); i++) {
    if ((p.ops[i].type == Operation::Type::DIV ||
         p.ops[i].type == Operation::Type::DIF ||
         p.ops[i].type == Operation::Type::MOD) &&
        (p.ops[i].source.type == Operand::Type::DIRECT)) {
      auto divisor = p.ops[i].source;
      p.ops.insert(p.ops.begin() + i,
                   Operation(Operation::Type::MOV, tmp, divisor));
      p.ops.insert(p.ops.begin() + i + 1,
                   Operation(Operation::Type::CMP, tmp,
                             Operand(Operand::Type::CONSTANT, 0)));
      p.ops.insert(p.ops.begin() + i + 2,
                   Operation(Operation::Type::ADD, divisor, tmp));
      i += 3;
    } else if (p.ops[i].type == Operation::Type::POW) {
      if (p.ops[i].source.type == Operand::Type::CONSTANT &&
          (p.ops[i].source.value < Number(2) ||
           Number(max_exponent) < p.ops[i].source.value)) {
        p.ops[i].source.value = (Random::get().gen() % (max_exponent - 2)) + 2;
      } else if (p.ops[i].source.type == Operand::Type::DIRECT &&
                 Random::get().gen() % 5 > 0)  // magic number
      {
        p.ops[i].source.type = Operand::Type::CONSTANT;
      }
    } else if (p.ops[i].type == Operation::Type::SEQ) {
      auto target = p.ops[i].target;
      p.ops.insert(p.ops.begin() + i,
                   Operation(Operation::Type::MAX, target,
                             Operand(Operand::Type::CONSTANT, Number::ZERO)));
      i++;
    }
  }
}

void Generator::fixCalls(Program &p) {
  for (auto &op : p.ops) {
    if (op.type == Operation::Type::SEQ) {
      if (op.source.type != Operand::Type::CONSTANT ||
          (op.source.value < Number::ZERO ||
           !(op.source.value < Number(found_programs.size())) ||
           !found_programs[op.source.value.asInt()])) {
        op.source =
            Operand(Operand::Type::CONSTANT, Number(getRandomProgramId()));
      }
    }
  }
}

int64_t Generator::getRandomProgramId() {
  int64_t id;
  do {
    id = Random::get().gen() % found_programs.size();
  } while (!found_programs[id]);
  return id;
}

void Generator::ensureSourceNotOverwritten(Program &p) {
  // make sure that the initial value does not get overridden immediately
  bool resets;
  for (auto &op : p.ops) {
    if (op.target.type == Operand::Type::DIRECT &&
        op.target.value == Program::INPUT_CELL) {
      resets = false;
      if (op.type == Operation::Type::MOV || op.type == Operation::Type::CLR) {
        resets = true;
      } else if ((op.source == op.target) &&
                 (op.type == Operation::Type::SUB ||
                  op.type == Operation::Type::TRN ||
                  op.type == Operation::Type::DIV ||
                  op.type == Operation::Type::DIF ||
                  op.type == Operation::Type::MOD)) {
        resets = true;
      }
      if (resets) {
        op.target.value = (Random::get().gen() % 4) + 1;
      }
    } else if (op.source.type == Operand::Type::DIRECT &&
               op.source.value == Program::INPUT_CELL) {
      break;
    }
  }
}

void Generator::ensureTargetWritten(Program &p,
                                    const std::vector<int64_t> &written_cells) {
  // make sure that the target value gets written
  bool written = false;
  for (auto &op : p.ops) {
    if (op.type != Operation::Type::LPB &&
        Operation::Metadata::get(op.type).num_operands == 2 &&
        op.target.type == Operand::Type::DIRECT &&
        op.target.value == Program::OUTPUT_CELL) {
      written = true;
      break;
    }
  }
  if (!written) {
    int64_t source = Program::INPUT_CELL;
    if (!written_cells.empty()) {
      source = written_cells.at(Random::get().gen() % written_cells.size());
    }
    p.ops.push_back(
        Operation(Operation::Type::MOV,
                  Operand(Operand::Type::DIRECT, Program::OUTPUT_CELL),
                  Operand(Operand::Type::DIRECT, source)));
  }
}

void Generator::ensureMeaningfulLoops(Program &p) {
  // make sure loops do something
  Operand mem;
  int64_t num_ops = 0;
  bool can_descent = false;
  for (size_t i = 0; i < p.ops.size(); i++) {
    switch (p.ops[i].type) {
      case Operation::Type::LPB: {
        mem = p.ops[i].target;
        can_descent = false;
        num_ops = 0;
        break;
      }
      case Operation::Type::ADD:
      case Operation::Type::MUL:
      case Operation::Type::POW:
        num_ops++;
        break;
      case Operation::Type::SUB:
      case Operation::Type::MOV:
      case Operation::Type::DIV:
      case Operation::Type::DIF:
      case Operation::Type::MOD:
      case Operation::Type::GCD:
      case Operation::Type::BIN:
      case Operation::Type::CMP:
        num_ops++;
        if (p.ops[i].target == mem) {
          can_descent = true;
        }
        break;
      case Operation::Type::LPE: {
        if (!can_descent) {
          Operation dec;
          dec.target = mem;
          dec.source =
              Operand(Operand::Type::CONSTANT, (Random::get().gen() % 9) + 1);
          switch (Random::get().gen() % 4) {
            case 0:
              dec.type = Operation::Type::TRN;
              break;
            case 1:
              dec.type = Operation::Type::DIV;
              dec.source.value = Semantics::add(dec.source.value, 1);
              break;
            case 2:
              dec.type = Operation::Type::DIF;
              dec.source.value = Semantics::add(dec.source.value, 1);
              break;
            case 3:
              dec.type = Operation::Type::MOD;
              dec.source.value = Semantics::add(dec.source.value, 1);
              break;
          }
          p.ops.insert(p.ops.begin() + i, dec);
          i++;
        }
        if (num_ops < 2) {
          for (int64_t j = (Random::get().gen() % 3) + 3; j > 0; j--) {
            auto op = generateOperation();
            if (op.first.type != Operation::Type::LPB &&
                op.first.type != Operation::Type::LPE) {
              p.ops.insert(p.ops.begin() + i, op.first);
              i++;
            }
          }
        }
        break;
      }
      default:
        break;
    }
  }
}

MultiGenerator::MultiGenerator(const Settings &settings, const Stats &stats,
                               bool print_info)
    : scheduler(60)  // 1 minute
{
  const auto config = ConfigLoader::load(settings);
  configs.clear();
  generators.clear();
  for (auto c : config.generators) {
    try {
      auto gen = Generator::Factory::createGenerator(c, stats);
      generators.emplace_back(std::move(gen));
      configs.push_back(c);
    } catch (std::exception &) {
      Log::get().warn("Ignoring error while loading generator");
    }
  }
  if (generators.empty()) {
    Log::get().error("No valid generators configurations found", true);
  }
  generator_index = Random::get().gen() % configs.size();

  // print info
  if (print_info) {
    std::string overwrite;
    switch (config.overwrite_mode) {
      case OverwriteMode::NONE:
        overwrite = "none";
        break;
      case OverwriteMode::ALL:
        overwrite = "all";
        break;
      case OverwriteMode::AUTO:
        overwrite = "auto";
        break;
    }
    Log::get().info("Initialized " + std::to_string(generators.size()) +
                    " generators (profile: " + config.name +
                    ", overwrite: " + overwrite + ")");
  }
}

Generator *MultiGenerator::getGenerator() {
  return generators[generator_index].get();
}

void MultiGenerator::next() {
  if (generators.size() > 1 && scheduler.isTargetReached()) {
    generator_index = (generator_index + 1) % configs.size();
    // Log::get().debug( "Using generator " + std::to_string( generator_index )
    // );
    scheduler.reset();
  }
}
