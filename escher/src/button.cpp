#include <escher/button.h>
#include <escher/palette.h>
#include <assert.h>

Button::Button(Responder * parentResponder, I18n::Message textBody, Invocation invocation, const KDFont * font, KDColor textColor) :
  HighlightCell(),
  Responder(parentResponder),
  m_messageTextView(font, textBody, 0.5f, 0.5f, textColor, Palette::ButtonBackground),
  m_invocation(invocation),
  m_font(font),
  m_embossedRoundedStyle(false)
{
}

void Button::setMessage(I18n::Message message) {
  m_messageTextView.setMessage(message);
}

void Button::setEmbossedRoundedStyle(bool rounded) {
  if (m_embossedRoundedStyle != rounded) {
    m_embossedRoundedStyle = rounded;
    markRectAsDirty(bounds());
  }
}

int Button::numberOfSubviews() const {
  return m_embossedRoundedStyle ? 0 : 1;
}

View * Button::subviewAtIndex(int index) {
  assert(index == 0);
  return &m_messageTextView;
}

void Button::layoutSubviews(bool force) {
  m_messageTextView.setFrame(bounds(), force);
}

bool Button::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    m_invocation.perform(this);
    return true;
  }
  return false;
}

void Button::setHighlighted(bool highlight) {
  HighlightCell::setHighlighted(highlight);
  if (!m_embossedRoundedStyle) {
    KDColor backgroundColor = highlight? highlightedBackgroundColor() : Palette::ButtonBackground;
    m_messageTextView.setBackgroundColor(backgroundColor);
  }
  markRectAsDirty(bounds());
}

void Button::drawRect(KDContext * ctx, KDRect rect) const {
  if (!m_embossedRoundedStyle) {
    return;
  }
  KDColor backgroundColor = isHighlighted() ? Palette::ButtonBackgroundSelectedHighContrast : Palette::ButtonBackground;
  KDRect buttonBounds = bounds();
  ctx->fillRoundedRect(buttonBounds, k_embossedCornerRadius, Palette::ButtonRowBorder, Palette::ButtonBorderOut);
  KDRect innerBounds(1, 1, buttonBounds.width() - 2, buttonBounds.height() - 2);
  if (!innerBounds.isEmpty()) {
    KDCoordinate innerRadius = k_embossedCornerRadius > 1 ? k_embossedCornerRadius - 1 : 0;
    ctx->fillRoundedRect(innerBounds, innerRadius, backgroundColor, Palette::ButtonRowBorder);
  }
  const char * text = m_messageTextView.text();
  if (text == nullptr) {
    return;
  }
  KDSize textSize = m_font->stringSize(text);
  KDPoint origin(
      (bounds().width() - textSize.width()) / 2,
      (bounds().height() - textSize.height()) / 2);
  ctx->drawString(text, origin, m_font, Palette::ButtonText, backgroundColor);
}

KDSize Button::minimalSizeForOptimalDisplay() const {
  KDSize textSize = m_messageTextView.minimalSizeForOptimalDisplay();
  return KDSize(textSize.width() + (m_font == KDFont::SmallFont ? k_horizontalMarginSmall : k_horizontalMarginLarge), textSize.height() + k_verticalMargin);
}
