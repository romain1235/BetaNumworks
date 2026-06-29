#include "integer_list_controller.h"
#include <poincare/based_integer.h>
#include <poincare/opposite.h>
#include <poincare/integer.h>
#include <poincare/logarithm.h>
#include <poincare/empty_layout.h>
#include <poincare/factor.h>
#include <poincare/float.h>
#include "../app.h"
#include "../../shared/poincare_helpers.h"

using namespace Poincare;
using namespace Shared;

namespace Calculation {

Integer::Base baseAtIndex(int index) {
  switch (index) {
    case 2:
      return Integer::Base::Decimal;
    case 3:
      return Integer::Base::Hexadecimal;
    default:
      assert(index == 4);
      return Integer::Base::Binary;
  }
}

void IntegerListController::setExpression(Poincare::Expression e) {
  ExpressionsListController::setExpression(e);
  static_assert(k_maxNumberOfRows >= k_indexOfFactorExpression + 1, "k_maxNumberOfRows must be greater than k_indexOfFactorExpression");
  assert(!m_expression.isUninitialized() && m_expression.type() == ExpressionNode::Type::BasedInteger || (m_expression.type() == ExpressionNode::Type::Opposite && m_expression.childAtIndex(0).type() == ExpressionNode::Type::BasedInteger));
  assert(!m_expression.isUninitialized());

  Preferences * preferences = Preferences::sharedPreferences();
  float value = PoincareHelpers::ApproximateToScalar<float>(m_expression, App::app()->localContext());
  Float<float> floatExpression = Float<float>::Builder(value);
  int numberOfSignificantDigits = preferences->numberOfSignificantDigits();
  m_layouts[0] = floatExpression.createLayout(Preferences::PrintFloatMode::Scientific, numberOfSignificantDigits);
  m_layouts[1] = floatExpression.createLayout(Preferences::PrintFloatMode::Engineering, numberOfSignificantDigits);

  if (m_expression.type() == ExpressionNode::Type::BasedInteger) {
    Integer integer = static_cast<BasedInteger &>(m_expression).integer();
    for (int index = k_indexOfFirstBaseExpression; index < k_indexOfFactorExpression; ++index) {
      m_layouts[index] = integer.createLayout(baseAtIndex(index));
    }
  }
  else
  {
    Opposite b = static_cast<Opposite &>(m_expression);
    Expression e = b.childAtIndex(0);
    Integer childInt = static_cast<BasedInteger &>(e).integer();
    childInt.setNegative(true);
    Integer num_bits = Integer::CeilingLog2(childInt);
    Integer integer = Integer::TwosComplementToBits(childInt, num_bits);
    for (int index = k_indexOfFirstBaseExpression; index < k_indexOfFactorExpression; ++index) {
      if(baseAtIndex(index) == Integer::Base::Decimal) {
        m_layouts[index] = childInt.createLayout(baseAtIndex(index));
      } else {
        m_layouts[index] = integer.createLayout(baseAtIndex(index));
      }
    }
  }
  // Computing factorExpression
  Expression factor = Factor::Builder(m_expression.clone());
  PoincareHelpers::Simplify(&factor, App::app()->localContext(), ExpressionNode::ReductionTarget::User);
  if (!factor.isUndefined()) {
    m_layouts[k_indexOfFactorExpression] = PoincareHelpers::CreateLayout(factor);
  }
}

I18n::Message IntegerListController::messageAtIndex(int index) {
  switch (index) {
    case 0:
      return I18n::Message::Scientific;
    case 1:
      return I18n::Message::Engineering;
    case 2:
      return I18n::Message::DecimalBase;
    case 3:
      return I18n::Message::HexadecimalBase;
    case 4:
      return I18n::Message::BinaryBase;
    default:
      return I18n::Message::PrimeFactors;
  }
}

}
