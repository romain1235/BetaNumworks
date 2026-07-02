#include <escher/rounded_button.h>
#include <assert.h>

RoundedButton::RoundedButton(
    Responder * parentResponder,
    I18n::Message textBody,
    Invocation invocation,
    const KDFont * font) :
  HighlightCell(),
  Responder(parentResponder),
  m_messageTextView(font, textBody, 0.5f, 0.5f, Palette::ButtonText, Palette::ButtonBackground),
  m_invocation(invocation),
  m_font(font)
{
}

void RoundedButton::setMessage(I18n::Message message) {
  m_messageTextView.setMessage(message);
}

bool RoundedButton::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    m_invocation.perform(this);
    return true;
  }
  return false;
}

void RoundedButton::setHighlighted(bool highlight) {
  HighlightCell::setHighlighted(highlight);
  markRectAsDirty(bounds());
}

KDSize RoundedButton::minimalSizeForOptimalDisplay() const {
  KDSize textSize = m_messageTextView.minimalSizeForOptimalDisplay();
  return KDSize(textSize.width() + 2 * k_horizontalMargin, textSize.height() + k_verticalMargin);
}

void RoundedButton::drawRect(KDContext * ctx, KDRect rect) const {
  KDColor backgroundColor = isHighlighted() ? Palette::ButtonBackgroundSelectedHighContrast : Palette::ButtonBackground;
  ctx->fillRoundedRect(bounds(), k_cornerRadius, backgroundColor, Palette::PopUpBackground);
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
