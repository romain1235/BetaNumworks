#include "rational_list_controller.h"
#include "../app.h"
#include "../../shared/poincare_helpers.h"
#include <poincare/float.h>
#include <poincare_nodes.h>
#include <string.h>

using namespace Poincare;
using namespace Shared;

namespace Calculation {

Integer extractInteger(const Expression e) {
  assert(e.type() == ExpressionNode::Type::BasedInteger);
  return static_cast<const BasedInteger &>(e).integer();
}

void RationalListController::setExpression(Poincare::Expression e) {
  ExpressionsListController::setExpression(e);
  assert(!m_expression.isUninitialized());
  static_assert(k_maxNumberOfRows >= 4, "k_maxNumberOfRows must be greater than 4");

  bool negative = false;
  Expression div = m_expression;
  if (m_expression.type() == ExpressionNode::Type::Opposite) {
    negative = true;
    div = m_expression.childAtIndex(0);
  }

  assert(div.type() == ExpressionNode::Type::Division);
  Integer numerator = extractInteger(div.childAtIndex(0));
  numerator.setNegative(negative);
  Integer denominator = extractInteger(div.childAtIndex(1));

  int index = 0;
  Preferences * preferences = Preferences::sharedPreferences();
  float value = PoincareHelpers::ApproximateToScalar<float>(m_expression, App::app()->localContext());
  Float<float> floatExpression = Float<float>::Builder(value);
  int numberOfSignificantDigits = preferences->numberOfSignificantDigits();
  m_layouts[index++] = floatExpression.createLayout(Preferences::PrintFloatMode::Scientific, numberOfSignificantDigits);
  m_layouts[index++] = floatExpression.createLayout(Preferences::PrintFloatMode::Engineering, numberOfSignificantDigits);
  m_layouts[index++] = PoincareHelpers::CreateLayout(Integer::CreateMixedFraction(numerator, denominator));
  m_layouts[index++] = PoincareHelpers::CreateLayout(Integer::CreateEuclideanDivision(numerator, denominator));
}

I18n::Message RationalListController::messageAtIndex(int index) {
  switch (index) {
    case 0:
      return I18n::Message::Scientific;
    case 1:
      return I18n::Message::Engineering;
    case 2:
      return I18n::Message::MixedFraction;
    default:
      return I18n::Message::EuclideanDivision;
  }
}

int RationalListController::textAtIndex(char * buffer, size_t bufferSize, int index) {
  int length = ExpressionsListController::textAtIndex(buffer, bufferSize, index);
  if (index == 3) {
    // Get rid of the left part of the equality
    char * equalPosition = strchr(buffer, '=');
    assert(equalPosition != nullptr);
    strlcpy(buffer, equalPosition + 1, bufferSize);
    return buffer + length - 1 - equalPosition;
  }
  return length;
}

}
