#include "form/formula_alt.hpp"

#include "form/expression_util.hpp"
#include "sys/log.hpp"

VariantsManager::VariantsManager(
    const Formula& formula,
    const std::map<std::string, int64_t>& num_initial_terms) {
  // step 1: collect function names
  for (auto& entry : formula.entries) {
    if (ExpressionUtil::isSimpleFunction(entry.first, true)) {
      variants[entry.first.name] = {};
    }
  }
  // step 2: initialize function variants
  for (auto& entry : formula.entries) {
    if (ExpressionUtil::isSimpleFunction(entry.first, true)) {
      Variant variant;
      variant.definition = entry.second;
      variant.num_initial_terms = num_initial_terms.at(entry.first.name);
      collectUsedFuncs(variant.definition, variant.used_funcs);
      variants[entry.first.name].push_back(variant);
    }
  }
}

bool VariantsManager::update(const std::string& func, const Expression& expr,
                             int64_t num_initial_terms) {
  Variant new_variant;
  new_variant.definition = expr;
  new_variant.num_initial_terms = num_initial_terms;
  collectUsedFuncs(expr, new_variant.used_funcs);
  if (new_variant.used_funcs.size() > 3) {  // magic number
    return false;
  }
  auto& vs = variants[func];
  for (size_t i = 0; i < vs.size(); i++) {
    if (vs[i].used_funcs == new_variant.used_funcs) {
      if (expr.numTerms() < vs[i].definition.numTerms()) {
        // update existing variant but don't report as new
        vs[i] = new_variant;
        Log::get().debug("Updated variant to " +
                         ExpressionUtil::newFunction(func).toString() + " = " +
                         expr.toString());
      }
      return false;
    }
  }
  // add new variant
  Log::get().debug("Found variant " +
                   ExpressionUtil::newFunction(func).toString() + " = " +
                   expr.toString());
  vs.push_back(new_variant);
  return true;
}

void VariantsManager::collectUsedFuncs(
    const Expression& expr, std::set<std::string>& used_funcs) const {
  if (expr.type == Expression::Type::FUNCTION &&
      variants.find(expr.name) != variants.end()) {
    used_funcs.insert(expr.name);
  }
  for (auto c : expr.children) {
    collectUsedFuncs(*c, used_funcs);
  }
}

size_t VariantsManager::numVariants() const {
  size_t num = 0;
  for (auto& vs : variants) {
    num += vs.second.size();
  }
  return num;
}

std::pair<bool, int64_t> resolve(const std::string& lookup_name,
                                 const Expression& lookup_def,
                                 int64_t lookup_initial_terms,
                                 const std::string& target_name,
                                 Expression& target_def,
                                 int64_t target_initial_terms) {
  std::pair<bool, int64_t> result(false, 0);
  if (target_def.type == Expression::Type::FUNCTION) {
    if (target_def.name != target_name && target_def.name == lookup_name) {
      auto replacement = lookup_def;       // copy
      auto arg = *target_def.children[0];  // copy
      replacement.replaceAll(ExpressionUtil::newParameter(), arg);
      ExpressionUtil::normalize(replacement);
      target_def = replacement;
      result.first = true;
      result.second =
          std::max(target_initial_terms,
                   lookup_initial_terms -
                       ExpressionUtil::eval(arg, {{"n", 0}}).asInt() - 1);
      Log::get().debug("TARGET: " + std::to_string(target_initial_terms) +
                       ", LOOKUP: " + std::to_string(lookup_initial_terms) +
                       ", EXPR: " + arg.toString() +
                       ", RESULT: " + std::to_string(result.second));
      return result;  // must stop here
    }
  }
  for (auto c : target_def.children) {
    auto r = resolve(lookup_name, lookup_def, lookup_initial_terms, target_name,
                     *c, target_initial_terms);
    if (r.first) {
      result.first = true;
      result.second = std::max(result.second, r.second);
    }
  }
  ExpressionUtil::normalize(target_def);
  return result;
}

bool findVariants(VariantsManager& manager) {
  auto variants = manager.variants;  // copy
  bool updated = false;
  for (auto& target : variants) {
    for (auto& target_variant : target.second) {
      for (auto& lookup : variants) {
        for (auto& lookup_variant : lookup.second) {
          auto def = target_variant.definition;  // copy
          auto r = resolve(lookup.first, lookup_variant.definition,
                           lookup_variant.num_initial_terms, target.first, def,
                           target_variant.num_initial_terms);
          if (r.first && manager.update(target.first, def, r.second)) {
            updated = true;
          }
        }
      }
    }
  }
  return updated;
}

bool simplifyFormulaUsingVariants(
    Formula& formula, std::map<std::string, int64_t>& num_initial_terms) {
  VariantsManager manager(formula, num_initial_terms);
  bool found = false;
  for (size_t it = 1; it <= 10; it++) {  // magic number
    Log::get().debug("Finding variants in iteration " + std::to_string(it));
    if (findVariants(manager)) {
      found = true;
    } else {
      break;
    }
  }
  if (!found) {
    return false;
  }
  Log::get().debug("Found " + std::to_string(manager.numVariants()) +
                   " variants");
  bool applied = false;
  for (auto& entry : formula.entries) {
    if (!ExpressionUtil::isSimpleFunction(entry.first, true)) {
      continue;
    }
    for (auto& variant : manager.variants[entry.first.name]) {
      if (variant.definition == entry.second) {
        continue;
      }
      Formula copy = formula;
      copy.entries[entry.first] = variant.definition;
      auto deps_old = formula.getFunctionDeps(true, true);
      auto deps_new = copy.getFunctionDeps(true, true);
      std::string debugMsg = " variant " + entry.first.toString() + " = " +
                             variant.definition.toString();
      if (deps_new.size() < deps_old.size()) {
        entry.second = variant.definition;
        num_initial_terms[entry.first.name] = variant.num_initial_terms;
        applied = true;
        Log::get().debug("Applied" + debugMsg);
      } else {
        // Log::get().debug("Skipped" + debugMsg);
      }
    }
  }
  Log::get().debug("Updated formula:  " + formula.toString());
  return applied;
}
