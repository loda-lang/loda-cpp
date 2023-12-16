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

/*
bool replace(const std::string& lookup_name, const Expression& lookup_def,
             const std::string& target_name, Expression& target_def) {
  if (!ExpressionUtil::isSimpleFunction(target_def, false)) {
    return false;
  }
  if (lookup_name != target_def.name) {
    return false;
  }
  // should only consider local functions
  std::set<std::string> self, used_funcs;
  self.insert(lookup_name);
  ExpressionUtil::collectNames(lookup_def, Expression::Type::FUNCTION,
                               used_funcs);
  if (used_funcs != self) {
    return false;
  }
  Log::get().debug("CANDIDATE: " + target_name +
                   "(n)=" + target_def.toString() + " using " + lookup_name +
                   "(n)=" + lookup_def.toString());
  auto new_def = lookup_def;
  new_def.replaceName(lookup_name, target_name);
  new_def.replaceAll(ExpressionUtil::newParameter(), *target_def.children[0]);
  ExpressionUtil::normalize(new_def);
  Log::get().debug("NEW DEF: " + new_def.toString());
  target_def = new_def;
  return true;
}
*/

bool findVariants(VariantsManager& manager) {
  auto variants = manager.variants;  // copy
  bool updated = false;
  for (auto& lookup : variants) {
    for (auto& lookup_variant : lookup.second) {
      for (auto& target : variants) {
        for (auto& target_variant : target.second) {
          auto def = target_variant.definition;  // copy
          if (resolve(lookup.first, lookup_variant.definition, target.first,
                      def)) {
            if (manager.update(target.first, def)) {
              updated = true;
            }
          }
          /*
          def = target_variant.definition;  // copy
          if (replace(lookup.first, lookup_variant.definition, target.first,
                      def)) {
            if (manager.update(target.first, def)) {
              updated = true;
            }
          }
          */
        }
      }
    }
  }
  return updated;
}

bool simplifyFormulaUsingVariants(Formula& formula) {
  VariantsManager manager(formula);
  size_t iteration = 1;
  while (true) {
    Log::get().debug("Finding variants in iteration " +
                     std::to_string(iteration));
    if (!findVariants(manager)) {
      break;
    }
    iteration++;
  }
  Log::get().debug("Found " + std::to_string(manager.numVariants()) +
                   " variants");
  if (iteration == 1) {  // no varaints were found
    return false;
  }
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
