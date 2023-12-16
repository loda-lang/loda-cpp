#include "form/formula_alt.hpp"

#include "form/expression_util.hpp"
#include "sys/log.hpp"

VariantsManager::VariantsManager(const Formula& formula) {
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
      collectUsedFuncs(variant.definition, variant.used_funcs);
      variants[entry.first.name].push_back(variant);
    }
  }
}

bool VariantsManager::update(const std::string& func, const Expression& expr) {
  Variant new_variant;
  new_variant.definition = expr;
  collectUsedFuncs(expr, new_variant.used_funcs);
  auto& vs = variants[func];
  for (size_t i = 0; i < vs.size(); i++) {
    if (vs[i].used_funcs == new_variant.used_funcs) {
      if (expr.numTerms() < vs[i].definition.numTerms()) {
        // update existing variant
        vs[i].definition = expr;
        Log::get().debug("Updated variant to " +
                         ExpressionUtil::newFunction(func).toString() + " = " +
                         expr.toString());
        return true;
      } else {
        // not better than existing variant
        return false;
      }
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

bool resolve(const std::string& lookup_name, const Expression& lookup_def,
             const std::string& target_name, Expression& target_def) {
  if (target_def.type == Expression::Type::FUNCTION) {
    if (target_def.name != target_name && target_def.name == lookup_name) {
      auto replacement = lookup_def;  // copy
      replacement.replaceAll(ExpressionUtil::newParameter(),
                             *target_def.children[0]);
      ExpressionUtil::normalize(replacement);
      target_def = replacement;
      return true;  // must stop here
    }
  }
  bool resolved = false;
  for (auto c : target_def.children) {
    if (resolve(lookup_name, lookup_def, target_name, *c)) {
      resolved = true;
    }
  }
  ExpressionUtil::normalize(target_def);
  return resolved;
}

bool findVariants(VariantsManager& manager) {
  auto variants = manager.variants;  // copy
  bool updated = false;
  for (auto& target : variants) {
    for (auto& target_variant : target.second) {
      for (auto& lookup : variants) {
        for (auto& lookup_variant : lookup.second) {
          auto def = target_variant.definition;  // copy
          if (resolve(lookup.first, lookup_variant.definition, target.first,
                      def)) {
            if (manager.update(target.first, def)) {
              updated = true;
            }
          }
        }
      }
    }
  }
  return updated;
}

bool simplifyFormulaUsingVariants(Formula& formula) {
  VariantsManager manager(formula);
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
