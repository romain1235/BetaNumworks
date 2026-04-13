#include "expression_with_equal_sign_view.h"
#include <poincare/code_point_layout.h>

namespace Calculation {

KDSize ExpressionWithEqualSignView::minimalSizeForOptimalDisplay() const {
  KDSize expressionSize = ExpressionView::minimalSizeForOptimalDisplay();
  KDSize equalSize = m_equalSign.minimalSizeForOptimalDisplay();
  return KDSize(expressionSize.width() + equalSize.width() + Metric::CommonLargeMargin, expressionSize.height());
}

void ExpressionWithEqualSignView::drawRect(KDContext * ctx, KDRect rect) const {
  if (m_layout.isUninitialized()) {
    return;
  }
  // Do not color the whole background to avoid coloring behind the equal symbol
  KDSize expressionSize = ExpressionView::minimalSizeForOptimalDisplay();
  ctx->fillRect(KDRect(0, 0, expressionSize), m_backgroundColor);
  // Temporarily enforce spacing matching editing state during draw.
  KDCoordinate previousSpacing = Poincare::ThousandsGroupingSpacing();
  KDCoordinate desiredSpacing = previousSpacing;
  if (m_isEditing != nullptr) {
    desiredSpacing = (*m_isEditing) ? 0 : 3;
  }
  if (previousSpacing != desiredSpacing) {
    Poincare::SetThousandsGroupingSpacing(desiredSpacing);
  }
  m_layout.draw(ctx, drawingOrigin(), m_textColor, m_backgroundColor, m_selectionStart, m_selectionEnd, Palette::Select);
  if (previousSpacing != desiredSpacing) {
    Poincare::SetThousandsGroupingSpacing(previousSpacing);
  }
}

View * ExpressionWithEqualSignView::subviewAtIndex(int index) {
  assert(index == 0);
  return &m_equalSign;
}

void ExpressionWithEqualSignView::layoutSubviews(bool force) {
  KDSize expressionSize = ExpressionView::minimalSizeForOptimalDisplay();
  KDSize equalSize = m_equalSign.minimalSizeForOptimalDisplay();
  KDCoordinate expressionBaseline = layout().baseline();
  m_equalSign.setFrame(KDRect(expressionSize.width() + Metric::CommonLargeMargin, expressionBaseline - equalSize.height()/2, equalSize), force);
}

}
