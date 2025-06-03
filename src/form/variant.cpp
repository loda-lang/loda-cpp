#include "form/variant.hpp"

#include "form/expression_util.hpp"
#include "form/formula_util.hpp"
#include "sys/log.hpp"

VariantsManager::VariantsManager(
    const Formula& formula,
    const std::map<std::string, int64_t>& num_initial_terms) {
  // step 1: collect function names
  for (const auto& entry : formula.entries) {
    if (ExpressionUtil::isSimpleFunction(entry.first, true)) {
      variants[entry.first.name] = {};
    }
  }
  // step 2: initialize function variants
  for (const auto& entry : formula.entries) {
    if (ExpressionUtil::isSimpleFunction(entry.first, true)) {
      Variant variant;
      variant.func = entry.first.name;
      variant.definition = entry.second;
      variant.num_initial_terms = num_initial_terms.at(entry.first.name);
      collectFuncs(variant);
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
  // ignore trivial variants
  if (ExpressionUtil::isSimpleFunction(new_variant.definition) &&
      new_variant.definition.name == new_variant.func) {
    return false;
  }
  collectFuncs(new_variant);
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

void VariantsManager::collectFuncs(Variant& variant) const {
  variant.used_funcs.clear();
  variant.required_funcs.clear();
  collectFuncs(variant, variant.definition);
}

void VariantsManager::collectFuncs(Variant& variant,
                                   const Expression& expr) const {
  if (expr.type == Expression::Type::FUNCTION &&
      variants.find(expr.name) != variants.end()) {
    variant.used_funcs.insert(expr.name);
    if (expr.children.size() == 1 &&
        expr.children.front().type == Expression::Type::PARAMETER) {
      variant.required_funcs.insert(expr.name);
    }
  }
  for (const auto& c : expr.children) {
    collectFuncs(variant, c);
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

bool gaussElim(const Variant& lookup, Variant& target) {
  if (target.definition.type != Expression::Type::SUM &&
      lookup.definition.type != Expression::Type::SUM) {
    return false;
  }
  if (target.func == lookup.func) {
    return false;
  }
  if (!target.definition.contains(Expression::Type::FUNCTION, lookup.func)) {
    return false;
  }
  if (!lookup.definition.contains(Expression::Type::FUNCTION, target.func)) {
    return false;
  }
  if (!lookup.required_funcs.empty()) {
    return false;
  }
  Expression negated(Expression::Type::PRODUCT, "",
                     {ExpressionUtil::newConstant(-1), lookup.definition});
  Expression replacement(
      Expression::Type::SUM, "",
      {target.definition, negated, ExpressionUtil::newFunction(lookup.func)});
  target.num_initial_terms =
      std::max(target.num_initial_terms, lookup.num_initial_terms) + 1;
  ExpressionUtil::normalize(replacement);
  // Log::get().debug("Gaussian elimination: " + target.func +
  //                  "(n)=" + target.definition.toString() + " & " +
  //                  lookup.func +
  //                  "(n)=" + lookup.definition.toString() + " => " +
  //                  target.func + "(n)=" + replacement.toString());
  target.definition = replacement;
  return true;
}

bool replaceSimple(const Variant& lookup, Variant& target) {
  if (!ExpressionUtil::isSimpleFunction(target.definition, false)) {
    return false;
  }
  if (target.definition.name == target.func ||
      target.definition.name != lookup.func) {
    return false;
  }

  // TODO: we should also make sure that exactly it's one parameter entry
  /*
    if (target.definition.type != Expression::Type::SUM) {
      return false;
    }
    const auto& summands = target.definition.children.front().children;
    if (std::any_of(summands.begin(), summands.end(), [&](const Expression& c) {
          return c.type != Expression::Type::CONSTANT &&
                 c.type != Expression::Type::PARAMETER;
        })) {
      return false;
    }
    */

  std::set<std::string> names;
  std::set<std::string> singleton = {lookup.func};
  ExpressionUtil::collectNames(lookup.definition, Expression::Type::FUNCTION,
                               names);
  if (names != singleton) {
    return false;
  }
  auto replacement = lookup.definition;  // copy
  replacement.replaceName(lookup.func, target.func);
  ExpressionUtil::normalize(replacement);
  Log::get().debug("Replace simple: " + target.func +
                   "(n)=" + target.definition.toString() + " & " + lookup.func +
                   "(n)=" + lookup.definition.toString() + " => " +
                   target.func + "(n)=" + replacement.toString());
  target.definition = replacement;
  return true;
}

bool findVariants(VariantsManager& manager) {
  auto variants = manager.variants;  // copy
  bool updated = false;
  for (const auto& target : variants) {
    for (const auto& target_variant : target.second) {
      for (const auto& lookup : variants) {
        for (const auto& lookup_variant : lookup.second) {
          auto new_variant = target_variant;  // copy
          if (resolve(lookup_variant, new_variant) &&
              manager.update(new_variant)) {
            updated = true;
          }
          new_variant = target_variant;  // copy
          if (gaussElim(lookup_variant, new_variant) &&
              manager.update(new_variant)) {
            updated = true;
          }
          new_variant = target_variant;  // copy
          if (replaceSimple(lookup_variant, new_variant) &&
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
  while (manager.numVariants() < 200) {  // magic number
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
      if (!variant.required_funcs.empty()) {
        continue;
      }
      Formula copy = formula;
      copy.entries[entry.first] = variant.definition;
      auto deps_old = FormulaUtil::getDependencies(
          formula, Expression::Type::FUNCTION, true, true);
      auto deps_new = FormulaUtil::getDependencies(
          copy, Expression::Type::FUNCTION, true, true);
      if (deps_new.size() < deps_old.size()) {
        entry.second = variant.definition;
        num_initial_terms[entry.first.name] = variant.num_initial_terms;
        applied = true;
        debugUpdate("Applied variant ", variant);
      }
    }
  }
  Log::get().debug("Updated formula:  " + formula.toString());
  return applied;
}
