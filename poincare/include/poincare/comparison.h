#ifndef POINCARE_COMPARISON_H
#define POINCARE_COMPARISON_H

#include <poincare/expression.h>

namespace Poincare {

class ComparisonNode final : public ExpressionNode {
public:
  enum class Operator : uint8_t {
    Equal,
    NotEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual
  };

  explicit ComparisonNode(Operator op) : m_operator(op) {}

  // TreeNode
  size_t size() const override { return sizeof(ComparisonNode); }
  int numberOfChildren() const override { return 2; }
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream & stream) const override {
    stream << "Comparison";
  }
#endif

  // ExpressionNode
  Type type() const override { return Type::Comparison; }
  int polynomialDegree(Context * context, const char * symbolName) const override { return -1; }
  Operator op() const { return m_operator; }

private:
  Operator m_operator;
  Expression shallowReduce(ReductionContext reductionContext) override;
  Layout createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  int serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  Evaluation<float> approximate(SinglePrecision p, ApproximationContext approximationContext) const override { return templatedApproximate<float>(approximationContext); }
  Evaluation<double> approximate(DoublePrecision p, ApproximationContext approximationContext) const override { return templatedApproximate<double>(approximationContext); }
  template<typename T> Evaluation<T> templatedApproximate(ApproximationContext approximationContext) const;
  LayoutShape leftLayoutShape() const override { assert(false); return LayoutShape::BoundaryPunctuation; };
};

class Comparison final : public Expression {
public:
  Comparison(const ComparisonNode * n) : Expression(n) {}
  static Comparison Builder(ComparisonNode::Operator op, Expression child0, Expression child1);
  ComparisonNode::Operator op() const { return static_cast<const ComparisonNode *>(node())->op(); }
  Expression shallowReduce(ExpressionNode::ReductionContext reductionContext);
};

/* Returns -1, 0, 1 if a comparison order can be established, or 2 if not. */
int ComparisonOrder(Expression a, Expression b, ExpressionNode::ReductionContext reductionContext);

/* Returns 0 (false), 1 (true) or 2 if the expression has no boolean value. */
int BooleanValue(Expression e, ExpressionNode::ReductionContext reductionContext);

}

#endif
