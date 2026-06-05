#ifndef POINCARE_BOOLEAN_H
#define POINCARE_BOOLEAN_H

#include <poincare/number.h>

namespace Poincare {

class BooleanNode final : public NumberNode {
public:
  explicit BooleanNode(bool value) : m_value(value) {}

  // TreeNode
  size_t size() const override { return sizeof(BooleanNode); }
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream & stream) const override {
    stream << (m_value ? "BooleanTrue" : "BooleanFalse");
  }
#endif

  // Properties
  Type type() const override { return Type::Boolean; }
  bool isNumber() const override { return false; }
  int polynomialDegree(Context * context, const char * symbolName) const override { return 0; }
  Expression setSign(Sign s, ReductionContext reductionContext) override;

  bool value() const { return m_value; }

  // Approximation
  Evaluation<float> approximate(SinglePrecision p, ApproximationContext approximationContext) const override {
    return templatedApproximate<float>();
  }
  Evaluation<double> approximate(DoublePrecision p, ApproximationContext approximationContext) const override {
    return templatedApproximate<double>();
  }

  // Layout
  Layout createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  int serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode = Preferences::PrintFloatMode::Decimal, int numberOfSignificantDigits = 0) const override;
private:
  bool m_value;
  template<typename T> Evaluation<T> templatedApproximate() const;
  LayoutShape leftLayoutShape() const override { return LayoutShape::MoreLetters; };
};

class Boolean final : public Number {
public:
  Boolean(const BooleanNode * n) : Number(n) {}
  static Boolean Builder(bool value);
  bool value() const { return node()->value(); }
  static const char * TrueName() { return "true"; }
  static const char * FalseName() { return "false"; }
private:
  BooleanNode * node() const { return static_cast<BooleanNode *>(Expression::node()); }
};

}

#endif
