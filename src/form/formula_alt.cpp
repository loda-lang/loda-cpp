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
      variant.func = entry.first.name;
      variant.definition = entry.second;
      variant.num_initial_terms = num_initial_terms.at(entry.first.name);
      collectUsedFuncs(variant.definition, variant.used_funcs);
      variants[entry.first.name].push_back(variant);
    }
  }
}

void debugUpdate(const std::string& prefix, const Variant& variant) {
  Log::get().debug(prefix +
                   ExpressionUtil::newFunction(variant.func).toString() +
                   " = " + variant.definition.toString());
}

bool VariantsManager::update(Variant new_variant) {
  new_variant.used_funcs.clear();
  collectUsedFuncs(new_variant.definition, new_variant.used_funcs);
  const auto num_terms = new_variant.definition.numTerms();
  // if (new_variant.used_funcs.size() > 3) {  // magic number
  //   return false;
  // }
  auto& vs = variants[new_variant.func];
  // prevent rapid increases of variant sizes
  if (!std::all_of(vs.begin(), vs.end(), [new_variant](const Variant& v) {
        return v.used_funcs.size() + 1 >= new_variant.used_funcs.size();
      })) {
    return false;
  }
  for (size_t i = 0; i < vs.size(); i++) {
    if (vs[i].used_funcs == new_variant.used_funcs) {
      if (num_terms < vs[i].definition.numTerms()) {
        // update existing variant but don't report as new
        vs[i] = new_variant;
        debugUpdate("Updated variant to ", new_variant);
      }
      return false;
    }
  }
  // add new variant
  vs.push_back(new_variant);
  debugUpdate("Found variant ", new_variant);
  return true;
}

void VariantsManager::collectUsedFuncs(
    const Expression& expr, std::set<std::string>& used_funcs) const {
  if (expr.type == Expression::Type::FUNCTION &&
      variants.find(expr.name) != variants.end()) {
    used_funcs.insert(expr.name);
  }
  for (auto& c : expr.children) {
    collectUsedFuncs(c, used_funcs);
  }
}

size_t VariantsManager::numVariants() const {
  size_t num = 0;
  for (auto& vs : variants) {
    num += vs.second.size();
  }
  return num;
}

bool resolve(const Variant& lookup, Variant& target, Expression& target_def) {
  if (target_def.type == Expression::Type::FUNCTION &&
      target_def.children.size() == 1) {
    if (target_def.name != target.func && target_def.name == lookup.func) {
      auto replacement = lookup.definition;  // copy
      auto arg = target_def.children[0];     // copy
      // resolve function
      replacement.replaceAll(ExpressionUtil::newParameter(), arg);
      ExpressionUtil::normalize(replacement);
      target_def = replacement;
      // update number of required initial terms
      int64_t min_initial_terms =
          lookup.num_initial_terms -
          ExpressionUtil::eval(arg, {{"n", 0}}).asInt() - 1;
      target.num_initial_terms =
          std::max(target.num_initial_terms, min_initial_terms);
      // stop here, because else we would replace inside the replacement!
      return true;
    }
  }
  bool resolved = false;
  for (auto& c : target_def.children) {
    if (resolve(lookup, target, c)) {
      resolved = true;
    }
  }
  ExpressionUtil::normalize(target_def);
  return resolved;
}

bool resolve(const Variant& lookup, Variant& target) {
  return resolve(lookup, target, target.definition);
}

bool findVariants(VariantsManager& manager) {
  auto variants = manager.variants;  // copy
  bool updated = false;
  for (auto& target : variants) {
    for (auto& target_variant : target.second) {
      for (auto& lookup : variants) {
        for (auto& lookup_variant : lookup.second) {
          auto new_variant = target_variant;  // copy
          if (resolve(lookup_variant, new_variant) &&
              manager.update(new_variant)) {
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
