#ifndef ESCHER_ROUNDED_BUTTON_H
#define ESCHER_ROUNDED_BUTTON_H

#include <escher/highlight_cell.h>
#include <escher/i18n.h>
#include <escher/responder.h>
#include <escher/message_text_view.h>
#include <escher/invocation.h>
#include <escher/palette.h>

class RoundedButton : public HighlightCell, public Responder {
public:
  RoundedButton(
      Responder * parentResponder,
      I18n::Message textBody,
      Invocation invocation,
      const KDFont * font = KDFont::SmallFont);
  void setMessage(I18n::Message message);
  bool handleEvent(Ion::Events::Event event) override;
  void setHighlighted(bool highlight) override;
  Responder * responder() override {
    return this;
  }
  KDSize minimalSizeForOptimalDisplay() const override;
  void drawRect(KDContext * ctx, KDRect rect) const override;
private:
  constexpr static KDCoordinate k_cornerRadius = 4;
  constexpr static KDCoordinate k_verticalMargin = 5;
  constexpr static KDCoordinate k_horizontalMargin = 10;
  MessageTextView m_messageTextView;
  Invocation m_invocation;
  const KDFont * m_font;
};

#endif
