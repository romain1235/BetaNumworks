#ifndef CODE_CONSOLE_LINE_CELL_H
#define CODE_CONSOLE_LINE_CELL_H

#include <escher/highlight_cell.h>
#include <escher/message_text_view.h>
#include <escher/responder.h>
#include <escher/palette.h>
#include <assert.h>

#include "console_line.h"

namespace Code {

class ConsoleLineCell : public HighlightCell, public Responder {
public:
  ConsoleLineCell(Responder * parentResponder = nullptr);
  void setLine(ConsoleLine line);

  /* HighlightCell */
  void setHighlighted(bool highlight) override;
  void reloadCell() override;
  Responder * responder() override {
    return this;
  }
  const char * text() const override;
  /* View */
  int numberOfSubviews() const override;
  View * subviewAtIndex(int index) override;
  void layoutSubviews(bool force = false) override;

  /* Responder */
  bool handleEvent(Ion::Events::Event event) override;
private:
  class ConsoleLineTextView : public View {
  public:
    ConsoleLineTextView();
    void setLine(ConsoleLine * line);
    void setHighlighted(bool highlight);
    void resetHorizontalOffset();
    KDCoordinate horizontalOffset() const { return m_horizontalOffset; }
    KDCoordinate maxHorizontalOffset() const;
    void scrollHorizontally(KDCoordinate offset);
    void drawRect(KDContext * ctx, KDRect rect) const override;
    KDSize minimalSizeForOptimalDisplay() const override;
  private:
    ConsoleLine * m_line;
    KDCoordinate m_horizontalOffset;
    bool m_highlighted;
  };
  static KDColor textColor(ConsoleLine * line) {
    return line->isFromCurrentSession() ? Palette::CodeText : Palette::SecondaryText;
  }
  MessageTextView m_promptView;
  ConsoleLineTextView m_lineTextView;
  ConsoleLine m_line;
};

}

#endif
