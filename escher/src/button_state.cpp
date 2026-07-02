#include <escher/button_state.h>
#include <assert.h>
#include <algorithm>

View * ButtonState::subviewAtIndex(int index) {
  assert(index >= 0 && index < 2);
  if (index == 0) {
    return &m_messageTextView;
  }
  return &m_stateView;
}

void ButtonState::layoutSubviews(bool force) {
  KDSize textSize = Button::minimalSizeForOptimalDisplay();
  KDSize stateSize = m_stateView.minimalSizeForOptimalDisplay();
  KDCoordinate totalWidth = textSize.width() + stateSize.width();
  KDCoordinate margin = (bounds().width()-totalWidth)>>1;
  KDRect textRect = KDRect(margin, 0, textSize.width(), bounds().height());
  // State view will be centered
  KDRect stateRect = KDRect(textSize.width()+margin, k_verticalOffset, stateSize.width(), stateSize.height());

  m_messageTextView.setFrame(textRect, force);
  m_stateView.setFrame(stateRect, force);
}

void ButtonState::drawRect(KDContext * ctx, KDRect rect) const {
  KDColor backColor = isHighlighted() ? highlightedBackgroundColor() : Palette::ButtonBackground;
  ctx->fillRect(bounds(), backColor);
}

KDSize ButtonState::minimalSizeForOptimalDisplay() const {
  KDSize textSize = Button::minimalSizeForOptimalDisplay();
  KDSize stateSize = m_stateView.minimalSizeForOptimalDisplay();
  return KDSize(
    textSize.width() + stateSize.width() + k_stateMargin,
    std::max(textSize.height(), stateSize.height()));
}
