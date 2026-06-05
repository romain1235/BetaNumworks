#include <poincare/comparison.h>
#include <poincare/binary_operation.h>
#include <poincare/boolean.h>
#include <poincare/subtraction.h>
#include <poincare/parenthesis.h>
#include <poincare/opposite.h>
#include <poincare/multiplication.h>
#include <poincare/power.h>
#include <poincare/symbol.h>
#include <poincare/number.h>
#include <poincare/rational.h>
#include <poincare/undefined.h>
#include <cmath>

namespace Poincare {

constexpr int k_incomparable = 2;

static bool isInvalidReducedExpression(Expression e) {
  return e.isUndefined() || e.type() == ExpressionNode::Type::Unreal;
}

static int orderFromDifference(Expression diff) {
  if (isInvalidReducedExpression(diff)) {
    return k_incomparable;
  }
  if (diff.type() == ExpressionNode::Type::Rational && static_cast<const Rational &>(diff).isZero()) {
    return 0;
  }
  if (diff.isNumber()) {
    return Number::NaturalOrder(static_cast<const Number &>(diff), Rational::Builder(0));
  }
  return k_incomparable;
}

static Expression stripParenthesesAndOpposite(Expression e) {
  while (e.type() == ExpressionNode::Type::Parenthesis || e.type() == ExpressionNode::Type::Opposite) {
    e = e.childAtIndex(0);
  }
  return e;
}

static bool containsSymbol(Expression e, Context * context) {
  return e.recursivelyMatches([](const Expression expression, Context * ctx) {
    return expression.type() == ExpressionNode::Type::Symbol;
  }, context, ExpressionNode::SymbolicComputation::DoNotReplaceAnySymbol);
}

static bool isUnitFactor(Expression factor) {
  return factor.type() == ExpressionNode::Type::Rational
    && (static_cast<const Rational &>(factor).isOne() || static_cast<const Rational &>(factor).isMinusOne());
}

/* Detect a simplified difference that is not identically zero, such as a*b for
 * (a+b)^2-(a-b)^2-3*a*b. Simple differences like a-b stay incomparable. */
static int orderFromExpandedNonZeroDifference(Expression diff, ExpressionNode::ReductionContext reductionContext) {
  Expression e = stripParenthesesAndOpposite(diff);
  Context * context = reductionContext.context();
  if (e.type() == ExpressionNode::Type::Multiplication) {
    int nonUnitFactors = 0;
    bool hasSymbolFactor = false;
    for (int i = 0; i < e.numberOfChildren(); i++) {
      Expression factor = stripParenthesesAndOpposite(e.childAtIndex(i));
      if (isUnitFactor(factor)) {
        continue;
      }
      nonUnitFactors++;
      if (containsSymbol(factor, context)) {
        hasSymbolFactor = true;
      }
    }
    if (nonUnitFactors >= 2 && hasSymbolFactor) {
      return 1;
    }
  }
  if (e.type() == ExpressionNode::Type::Power) {
    Expression base = stripParenthesesAndOpposite(e.childAtIndex(0));
    Expression exponent = stripParenthesesAndOpposite(e.childAtIndex(1));
    if (containsSymbol(base, context)) {
      if (exponent.type() == ExpressionNode::Type::Rational) {
        if (!static_cast<const Rational &>(exponent).isZero()) {
          return 1;
        }
      } else {
        return 1;
      }
    }
  }
  return k_incomparable;
}

static int orderFromDifferenceOfReducedExpressions(Expression aRed, Expression bRed, ExpressionNode::ReductionContext reductionContext) {
  Expression diff = Subtraction::Builder(aRed.clone(), bRed.clone()).reduce(reductionContext);
  int order = orderFromDifference(diff);
  if (order != k_incomparable) {
    return order;
  }
  diff = Subtraction::Builder(aRed.clone(), bRed.clone()).simplify(reductionContext);
  if (isInvalidReducedExpression(diff)) {
    return k_incomparable;
  }
  order = orderFromDifference(diff);
  if (order != k_incomparable) {
    return order;
  }
  return orderFromExpandedNonZeroDifference(diff, reductionContext);
}

static int comparisonOrderReduced(Expression aRed, Expression bRed, ExpressionNode::ReductionContext reductionContext) {
  if (isInvalidReducedExpression(aRed) || isInvalidReducedExpression(bRed)) {
    return k_incomparable;
  }
  if (aRed.isIdenticalTo(bRed)) {
    return 0;
  }
  int order = orderFromDifferenceOfReducedExpressions(aRed, bRed, reductionContext);
  if (order != k_incomparable) {
    return order;
  }
  if (aRed.type() == ExpressionNode::Type::Boolean && bRed.type() == ExpressionNode::Type::Boolean) {
    bool aValue = static_cast<const Boolean &>(aRed).value();
    bool bValue = static_cast<const Boolean &>(bRed).value();
    if (aValue == bValue) {
      return 0;
    }
    return aValue ? 1 : -1;
  }
  if (aRed.isNumber() && bRed.isNumber()) {
    return Number::NaturalOrder(static_cast<const Number &>(aRed), static_cast<const Number &>(bRed));
  }
  double aScalar = aRed.approximateToScalar<double>(reductionContext.context(), reductionContext.complexFormat(), reductionContext.angleUnit(), true);
  double bScalar = bRed.approximateToScalar<double>(reductionContext.context(), reductionContext.complexFormat(), reductionContext.angleUnit(), true);
  if (std::isnan(aScalar) || std::isnan(bScalar)) {
    return k_incomparable;
  }
  if (aScalar < bScalar) {
    return -1;
  }
  if (aScalar > bScalar) {
    return 1;
  }
  return 0;
}

int ComparisonOrder(Expression a, Expression b, ExpressionNode::ReductionContext reductionContext) {
  Expression aRed = a.clone().reduce(reductionContext);
  Expression bRed = b.clone().reduce(reductionContext);
  int order = comparisonOrderReduced(aRed, bRed, reductionContext);
  if (order != k_incomparable) {
    return order;
  }
  Expression aSimp = a.clone().simplify(reductionContext);
  Expression bSimp = b.clone().simplify(reductionContext);
  if (aSimp.isUninitialized() || bSimp.isUninitialized()) {
    return k_incomparable;
  }
  return comparisonOrderReduced(aSimp, bSimp, reductionContext);
}

int BooleanValue(Expression e, ExpressionNode::ReductionContext reductionContext) {
  if (e.isUninitialized()) {
    return k_incomparable;
  }
  while (e.type() == ExpressionNode::Type::Parenthesis) {
    e = e.childAtIndex(0);
  }
  if (e.type() == ExpressionNode::Type::Boolean) {
    return static_cast<const Boolean &>(e).value() ? 1 : 0;
  }
  if (e.type() == ExpressionNode::Type::Comparison) {
    Expression r = static_cast<Comparison &>(e).shallowReduce(reductionContext);
    if (r.type() == ExpressionNode::Type::Boolean) {
      return static_cast<const Boolean &>(r).value() ? 1 : 0;
    }
    return k_incomparable;
  }
  if (e.type() == ExpressionNode::Type::And) {
    Expression r = static_cast<And &>(e).shallowReduce(reductionContext);
    if (r.type() == ExpressionNode::Type::Boolean) {
      return static_cast<const Boolean &>(r).value() ? 1 : 0;
    }
    return k_incomparable;
  }
  if (e.type() == ExpressionNode::Type::Or) {
    Expression r = static_cast<Or &>(e).shallowReduce(reductionContext);
    if (r.type() == ExpressionNode::Type::Boolean) {
      return static_cast<const Boolean &>(r).value() ? 1 : 0;
    }
    return k_incomparable;
  }
  if (e.type() == ExpressionNode::Type::Xor) {
    Expression r = static_cast<Xor &>(e).shallowReduce(reductionContext);
    if (r.type() == ExpressionNode::Type::Boolean) {
      return static_cast<const Boolean &>(r).value() ? 1 : 0;
    }
    return k_incomparable;
  }
  if (e.type() == ExpressionNode::Type::Not) {
    Expression r = static_cast<Not &>(e).shallowReduce(reductionContext);
    if (r.type() == ExpressionNode::Type::Boolean) {
      return static_cast<const Boolean &>(r).value() ? 1 : 0;
    }
    return k_incomparable;
  }
  Expression r = e.clone().reduce(reductionContext);
  if (r.isUninitialized() || r.isUndefined()) {
    return k_incomparable;
  }
  if (r.type() == ExpressionNode::Type::Boolean) {
    return static_cast<const Boolean &>(r).value() ? 1 : 0;
  }
  return k_incomparable;
}

}
