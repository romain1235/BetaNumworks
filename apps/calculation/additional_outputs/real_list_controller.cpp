#include "real_list_controller.h"
#include "../app.h"
#include "../../shared/poincare_helpers.h"
#include <poincare/float.h>

using namespace Poincare;
using namespace Shared;

namespace Calculation {

void RealListController::setExpression(Poincare::Expression e) {
  ExpressionsListController::setExpression(e);
  assert(!m_expression.isUninitialized());

  Context * context = App::app()->localContext();
  Preferences * preferences = Preferences::sharedPreferences();
  float value = PoincareHelpers::ApproximateToScalar<float>(m_expression, context);
  Float<float> floatExpression = Float<float>::Builder(value);
  int numberOfSignificantDigits = preferences->numberOfSignificantDigits();

  m_layouts[0] = floatExpression.createLayout(Preferences::PrintFloatMode::Scientific, numberOfSignificantDigits);
  m_layouts[1] = floatExpression.createLayout(Preferences::PrintFloatMode::Engineering, numberOfSignificantDigits);
}

I18n::Message RealListController::messageAtIndex(int index) {
  switch (index) {
    case 0:
      return I18n::Message::Scientific;
    default:
      return I18n::Message::Engineering;
  }
}

}
