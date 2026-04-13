#include <escher/expression_view.h>
#include <escher/palette.h>
#include <poincare/code_point_layout.h>
#include <stdio.h>
#include <algorithm>

using namespace Poincare;

ExpressionView::ExpressionView(float horizontalAlignment, float verticalAlignment,
    KDColor textColor, KDColor backgroundColor, Poincare::Layout * selectionStart, Poincare::Layout * selectionEnd, bool * isEditing ) :
  m_layout(),
  m_textColor(textColor),
  m_backgroundColor(backgroundColor),
  m_selectionStart(selectionStart),
  m_selectionEnd(selectionEnd),
  m_horizontalAlignment(horizontalAlignment),
  m_verticalAlignment(verticalAlignment),
  m_horizontalMargin(0)
{
  m_isEditing = isEditing;
}

bool ExpressionView::setLayout(Layout layoutR) {
  if (!m_layout.wasErasedByException() && m_layout.isIdenticalTo(layoutR)) {
    /* Check m_layout.wasErasedByException(), otherwise accessing m_layout would
     * result in an ACCESS ERROR. */
    return false;
  }
  int layoutId = layoutR.isUninitialized() ? -1 : layoutR.identifier();
  int isEditingVal = (m_isEditing != nullptr) ? (*m_isEditing ? 1 : 0) : -1;
  printf("[EV] setLayout id=%d isEditingPtr=%p isEditingVal=%d spacingBefore=%d\n", layoutId, (void *)m_isEditing, isEditingVal, (int)Poincare::ThousandsGroupingSpacing());
  // Ensure grouping spacing matches the editing state when assigning a layout
  KDCoordinate desiredSpacing = 3;
  if (m_isEditing != nullptr) {
    desiredSpacing = (*m_isEditing) ? 0 : 3;
  }
  KDCoordinate previousSpacing = Poincare::ThousandsGroupingSpacing();
  if (previousSpacing != desiredSpacing) {
    Poincare::SetThousandsGroupingSpacing(desiredSpacing);
    if (!layoutR.isUninitialized()) {
      layoutR.invalidAllSizesPositionsAndBaselines();
    }
  }
  printf("[EV] setLayout id=%d spacingAfter=%d\n", layoutId, (int)Poincare::ThousandsGroupingSpacing());
  m_layout = layoutR;
  markRectAsDirty(bounds());
  return true;
}

void ExpressionView::setBackgroundColor(KDColor backgroundColor) {
  if (m_backgroundColor != backgroundColor) {
    m_backgroundColor = backgroundColor;
    markRectAsDirty(bounds());
  }
}

void ExpressionView::setTextColor(KDColor textColor) {
  if (textColor != m_textColor) {
    m_textColor = textColor;
    markRectAsDirty(bounds());
  }
}

void ExpressionView::setAlignment(float horizontalAlignment, float verticalAlignment) {
  m_horizontalAlignment = horizontalAlignment;
  m_verticalAlignment = verticalAlignment;
  markRectAsDirty(bounds());
}

int ExpressionView::numberOfLayouts() const {
  return m_layout.numberOfDescendants(true);
}

KDSize ExpressionView::minimalSizeForOptimalDisplay() const {
  if (m_layout.isUninitialized()) {
    return KDSizeZero;
  }
  // Compute layout size using spacing appropriate to editing state to avoid
  // transient inconsistencies depending on global spacing order.
  KDCoordinate previousSpacing = Poincare::ThousandsGroupingSpacing();
  KDCoordinate desiredSpacing = previousSpacing;
  if (m_isEditing != nullptr) {
    desiredSpacing = (*m_isEditing) ? 0 : 3;
  }
  if (previousSpacing != desiredSpacing) {
    Poincare::SetThousandsGroupingSpacing(desiredSpacing);
  }
  KDSize expressionSize = m_layout.layoutSize();
  if (previousSpacing != desiredSpacing) {
    Poincare::SetThousandsGroupingSpacing(previousSpacing);
  }
  return KDSize(expressionSize.width() + 2*m_horizontalMargin, expressionSize.height());
}

KDPoint ExpressionView::drawingOrigin() const {
  // Ensure layoutSize is computed with correct spacing for editing state.
  KDCoordinate previousSpacing = Poincare::ThousandsGroupingSpacing();
  KDCoordinate desiredSpacing = previousSpacing;
  if (m_isEditing != nullptr) {
    desiredSpacing = (*m_isEditing) ? 0 : 3;
  }
  if (previousSpacing != desiredSpacing) {
    Poincare::SetThousandsGroupingSpacing(desiredSpacing);
  }
  KDSize expressionSize = m_layout.layoutSize();
  if (previousSpacing != desiredSpacing) {
    Poincare::SetThousandsGroupingSpacing(previousSpacing);
  }
  return KDPoint(m_horizontalMargin + m_horizontalAlignment*(m_frame.width() - 2*m_horizontalMargin - expressionSize.width()), std::max<KDCoordinate>(0, m_verticalAlignment*(m_frame.height() - expressionSize.height())));
}

KDPoint ExpressionView::absoluteDrawingOrigin() const {
  return drawingOrigin().translatedBy(m_frame.topLeft());
}

void ExpressionView::drawRect(KDContext * ctx, KDRect rect) const {
  ctx->fillRect(rect, m_backgroundColor);
  if (!m_layout.isUninitialized()) {
    // Temporarily enforce spacing matching editing state during draw to avoid
    // flickering depending on ordering of setLayout/setEditing.
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
}
