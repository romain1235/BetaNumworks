#include <poincare/comparison.h>
#include <poincare/boolean.h>
#include <poincare/horizontal_layout.h>
#include <poincare/code_point_layout.h>
#include <poincare/serialization_helper.h>
#include <poincare/tree_pool.h>
#include <poincare/complex.h>
#include <poincare/undefined.h>

namespace Poincare {

Comparison Comparison::Builder(ComparisonNode::Operator op, Expression child0, Expression child1) {
  TreePool * pool = TreePool::sharedPool();
  ComparisonNode * node = new (pool->alloc(sizeof(ComparisonNode))) ComparisonNode(op);
  TreeHandle h = TreeHandle::BuildWithGhostChildren(node);
  h.replaceChildAtIndexInPlace(0, child0);
  h.replaceChildAtIndexInPlace(1, child1);
  return static_cast<Comparison &>(h);
}

static const char * operatorSymbol(ComparisonNode::Operator op) {
  switch (op) {
  case ComparisonNode::Operator::Equal: return "=";
  case ComparisonNode::Operator::NotEqual: return "!=";
  case ComparisonNode::Operator::Less: return "<";
  case ComparisonNode::Operator::Greater: return ">";
  case ComparisonNode::Operator::LessEqual: return "<=";
  case ComparisonNode::Operator::GreaterEqual: return ">=";
  }
  assert(false);
  return "=";
}

static bool comparisonResult(ComparisonNode::Operator op, int order) {
  switch (op) {
  case ComparisonNode::Operator::Equal:
    return order == 0;
  case ComparisonNode::Operator::NotEqual:
    return order != 0;
  case ComparisonNode::Operator::Less:
    return order < 0;
  case ComparisonNode::Operator::Greater:
    return order > 0;
  case ComparisonNode::Operator::LessEqual:
    return order <= 0;
  case ComparisonNode::Operator::GreaterEqual:
    return order >= 0;
  }
  assert(false);
  return false;
}

Expression ComparisonNode::shallowReduce(ReductionContext reductionContext) {
  return Comparison(this).shallowReduce(reductionContext);
}

Layout ComparisonNode::createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  HorizontalLayout result = HorizontalLayout::Builder();
  result.addOrMergeChildAtIndex(childAtIndex(0)->createLayout(floatDisplayMode, numberOfSignificantDigits), 0, false);
  const char * symbol = operatorSymbol(m_operator);
  for (const char * c = symbol; *c != 0; c++) {
    result.addChildAtIndex(CodePointLayout::Builder(*c), result.numberOfChildren(), result.numberOfChildren(), nullptr);
  }
  result.addOrMergeChildAtIndex(childAtIndex(1)->createLayout(floatDisplayMode, numberOfSignificantDigits), result.numberOfChildren(), false);
  return std::move(result);
}

int ComparisonNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return SerializationHelper::Infix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, operatorSymbol(m_operator));
}

template<typename T>
Evaluation<T> ComparisonNode::templatedApproximate(ApproximationContext approximationContext) const {
  ExpressionNode::ReductionContext reductionContext(
      approximationContext.context(),
      approximationContext.complexFormat(),
      approximationContext.angleUnit(),
      Preferences::UnitFormat::Metric,
      ExpressionNode::ReductionTarget::SystemForApproximation);
  Comparison comparison(this);
  int order = ComparisonOrder(comparison.childAtIndex(0), comparison.childAtIndex(1), reductionContext);
  if (order == 2) {
    return Complex<T>::Undefined();
  }
  return Complex<T>::Builder(comparisonResult(m_operator, order) ? (T)1.0 : (T)0.0);
}

template Evaluation<float> ComparisonNode::templatedApproximate(ApproximationContext approximationContext) const;
template Evaluation<double> ComparisonNode::templatedApproximate(ApproximationContext approximationContext) const;

Expression Comparison::shallowReduce(ExpressionNode::ReductionContext reductionContext) {
  int order = ComparisonOrder(childAtIndex(0), childAtIndex(1), reductionContext);
  if (order == 2) {
    if (reductionContext.target() == ExpressionNode::ReductionTarget::User) {
      Expression result = Undefined::Builder();
      replaceWithInPlace(result);
      return result;
    }
    return *this;
  }
  Expression result = Boolean::Builder(comparisonResult(static_cast<const ComparisonNode *>(node())->op(), order));
  replaceWithInPlace(result);
  return result;
}

}
