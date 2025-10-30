#pragma once

#include "form/formula.hpp"

/**
 * Formula simplification utilities. This class provides static methods to
 * simplify mathematical formulas by resolving function identities, eliminating
 * simple function definitions, and replacing trivial recursive patterns with
 * direct expressions.
 */
class FormulaSimplify {
 public:
  /**
   * Resolves function identities in the formula.
   *
   * This method identifies and removes functions that are simple aliases
   * (identities) of other functions. For example, if we have:
   *   f(n) = g(n)
   *   g(n) = n + 1
   * It will remove f(n) and replace all references to f with g.
   *
   * @param formula The formula to simplify by resolving identities
   */
  static void resolveIdentities(Formula& formula);

  /**
   * Resolves simple function definitions by inlining them.
   *
   * This method identifies simple functions (functions with a single parameter
   * that don't have recursive dependencies) and replaces all their usages with
   * their definitions. For example, if we have:
   *   f(n) = 2*n + 1
   *   g(n) = f(n) + 3
   * It will transform g(n) to: g(n) = 2*n + 1 + 3 = 2*n + 4
   * and remove the definition of f(n).
   *
   * @param formula The formula to simplify by resolving simple functions
   */
  static void resolveSimpleFunctions(Formula& formula);

  /**
   * Replaces trivial linear recursions with direct formulas.
   *
   * This method identifies recursive functions that follow a simple linear
   * pattern f(n) = f(n-1) + c (arithmetic progression) and replaces them
   * with direct formulas f(n) = c*n + f(0). For example:
   *   f(0) = 5
   *   f(n) = f(n-1) + 3
   * Will be replaced with: f(n) = 3*n + 5
   *
   * @param formula The formula to simplify by replacing trivial recursions
   */
  static void replaceLinearRecursions(Formula& formula);

  /**
   * Replaces simple recursive function references with direct definitions.
   *
   * This method identifies functions that are simple references to other
   * recursive functions with an offset, and replaces them by copying the
   * referenced function's definition while adjusting indices. For example:
   *   a(n) = b(n-1)
   *   b(0) = 1, b(n) = 2*b(n-1)
   * Will be transformed to:
   *   a(0) = 1, a(n) = 2*a(n-1)
   * (removing b entirely if it's not used elsewhere)
   *
   * @param formula The formula to simplify by replacing simple recursive
   * references
   */
  static void replaceSimpleRecursiveRefs(Formula& formula);

  /**
   * Replaces geometric progressions with exponential formulas.
   *
   * This method identifies recursive functions that follow a geometric
   * progression pattern f(n) = c * f(n-1) and replaces them with direct
   * exponential formulas f(n) = a * c^n. For example:
   *   f(0) = 1
   *   f(n) = 2 * f(n-1)
   * Will be replaced with: f(n) = 2^n
   *
   * @param formula The formula to simplify by replacing geometric progressions
   */
  static void replaceGeometricProgressions(Formula& formula);
};