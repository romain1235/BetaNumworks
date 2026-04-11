#include "console_line_cell.h"
#include "console_controller.h"
#include <kandinsky/point.h>
#include <kandinsky/coordinate.h>
#include <apps/i18n.h>
#include <apps/global_preferences.h>

namespace Code {

ConsoleLineCell::ScrollableConsoleLineView::ConsoleLineView::ConsoleLineView() :
  HighlightCell(),
  m_line(nullptr)
{
}

void ConsoleLineCell::ScrollableConsoleLineView::ConsoleLineView::setLine(ConsoleLine * line) {
  m_line = line;
}

void ConsoleLineCell::ScrollableConsoleLineView::ConsoleLineView::drawRect(KDContext * ctx, KDRect rect) const {
  KDColor background = isHighlighted() ? Palette::Select : Palette::CodeBackground;
  ctx->fillRect(bounds(), Palette::CodeBackground);
  const char * text = m_line->text();
  const KDFont * font = GlobalPreferences::sharedGlobalPreferences()->font();
  KDCoordinate x = 0;
  KDColor defaultColor = textColor(m_line);
  // Parse escape sequences of the form "\x1b[C<r>,<g>,<b>;" ... "\x1b[0m"
  for (const char * p = text; *p != '\0'; ) {
    if (*p == '\x1b' && p[1] == '[' && p[2] == 'C') {
      // parse header r,g,b; starting at p+3
      const char * h = p + 3;
      int r = 0, g = 0, b = 0;
      int parsed = 0;
      // read r
      parsed = 0;
      while (*h >= '0' && *h <= '9') { r = r*10 + (*h - '0'); h++; parsed = 1; }
      if (*h == ',') h++; // skip comma
      // read g
      while (*h >= '0' && *h <= '9') { g = g*10 + (*h - '0'); h++; }
      if (*h == ',') h++;
      // read b
      while (*h >= '0' && *h <= '9') { b = b*10 + (*h - '0'); h++; }
      // skip terminating ';' if present
      if (*h == ';') h++;
      // now h points to start of segment text
      const char * segStart = h;
      // find terminating ESC sequence "\x1b[0m"
      const char * segEnd = segStart;
      while (*segEnd != '\0') {
        if (*segEnd == '\x1b' && segEnd[1] == '[' && segEnd[2] == '0' && segEnd[3] == 'm') {
          break;
        }
        segEnd++;
      }
      int segLen = segEnd - segStart;
      if (segLen > 0) {
        // draw this segment
        KDPoint point(x, 0);
        // temporary buffer for the segment (stack allocate max reasonable size)
        // avoid allocating if segLen is small
        if (segLen < 256) {
          char buf[256];
          memcpy(buf, segStart, segLen);
          buf[segLen] = '\0';
          KDSize s = font->stringSize(buf);
          KDColor color = KDColor::RGB888(r, g, b);
          ctx->drawString(buf, point, font, color, background);
          x += s.width();
        } else {
          // large segment: draw directly from pointer by temporarily null-terminating
          char save = segStart[segLen];
          ((char *)segStart)[segLen] = '\0';
          KDSize s = font->stringSize(segStart);
          KDColor color = KDColor::RGB888(r, g, b);
          ctx->drawString(segStart, point, font, color, background);
          ((char *)segStart)[segLen] = save;
          x += s.width();
        }
      }
      // advance p past the terminating sequence if any
      if (*segEnd == '\x1b') {
        p = segEnd + 4; // skip "\x1b[0m"
      } else {
        p = segEnd;
      }
    } else {
      // no escape: draw the remaining text normally
      KDPoint point(x, 0);
      KDSize s = font->stringSize(p);
      ctx->drawString(p, point, font, defaultColor, background);
      x += s.width();
      break;
    }
  }
}

KDSize ConsoleLineCell::ScrollableConsoleLineView::ConsoleLineView::minimalSizeForOptimalDisplay() const {
  // compute size ignoring escape sequences
  const char * text = m_line->text();
  const KDFont * font = GlobalPreferences::sharedGlobalPreferences()->font();
  KDCoordinate width = 0;
  for (const char * p = text; *p != '\0'; ) {
    if (*p == '\x1b' && p[1] == '[' && p[2] == 'C') {
      // skip header
      const char * h = p + 3;
      while (*h && *h != ';') { h++; }
      if (*h == ';') h++;
      // segment start
      const char * segStart = h;
      const char * segEnd = segStart;
      while (*segEnd != '\0') {
        if (*segEnd == '\x1b' && segEnd[1] == '[' && segEnd[2] == '0' && segEnd[3] == 'm') {
          break;
        }
        segEnd++;
      }
      int segLen = segEnd - segStart;
      if (segLen > 0) {
        if (segLen < 256) {
          char buf[256];
          memcpy(buf, segStart, segLen);
          buf[segLen] = '\0';
          width += font->stringSize(buf).width();
        } else {
          char save = segStart[segLen];
          ((char *)segStart)[segLen] = '\0';
          width += font->stringSize(segStart).width();
          ((char *)segStart)[segLen] = save;
        }
      }
      if (*segEnd == '\x1b') {
        p = segEnd + 4;
      } else {
        p = segEnd;
      }
    } else {
      width += font->stringSize(p).width();
      break;
    }
  }
  return KDSize(width, font->glyphSize().height());
}

ConsoleLineCell::ScrollableConsoleLineView::ScrollableConsoleLineView(Responder * parentResponder) :
  ScrollableView(parentResponder, &m_consoleLineView, this),
  m_consoleLineView()
{
}

ConsoleLineCell::ConsoleLineCell(Responder * parentResponder) :
  HighlightCell(),
  Responder(parentResponder),
  m_promptView(GlobalPreferences::sharedGlobalPreferences()->font(), I18n::Message::ConsolePrompt, 0, 0.5),
  m_scrollableView(this),
  m_line()
{
}

void ConsoleLineCell::setLine(ConsoleLine line) {
  m_line = line;
  m_scrollableView.consoleLineView()->setLine(&m_line);
  m_promptView.setTextColor(textColor(&m_line));
  reloadCell();
}

void ConsoleLineCell::setHighlighted(bool highlight) {
  HighlightCell::setHighlighted(highlight);
  m_scrollableView.consoleLineView()->setHighlighted(highlight);
}

void ConsoleLineCell::reloadCell() {
  layoutSubviews();
  HighlightCell::reloadCell();
  m_scrollableView.reloadScroll();
}

int ConsoleLineCell::numberOfSubviews() const {
  if (m_line.isCommand()) {
    return 2;
  }
  assert(m_line.isResult());
  return 1;
}

View * ConsoleLineCell::subviewAtIndex(int index) {
  if (m_line.isCommand()) {
    assert(index >= 0 && index < 2);
    View * views[] = {&m_promptView, &m_scrollableView};
    return views[index];
  }
  assert(m_line.isResult());
  assert(index == 0);
  return &m_scrollableView;
}

void ConsoleLineCell::layoutSubviews(bool force) {
  if (m_line.isCommand()) {
    KDSize promptSize = GlobalPreferences::sharedGlobalPreferences()->font()->stringSize(I18n::translate(I18n::Message::ConsolePrompt));
    m_promptView.setFrame(KDRect(KDPointZero, promptSize.width(), bounds().height()), force);
    m_scrollableView.setFrame(KDRect(KDPoint(promptSize.width(), 0), bounds().width() - promptSize.width(), bounds().height()), force);
    return;
  }
  assert(m_line.isResult());
  m_promptView.setFrame(KDRectZero, force);
  m_scrollableView.setFrame(bounds(), force);
}

void ConsoleLineCell::didBecomeFirstResponder() {
  Container::activeApp()->setFirstResponder(&m_scrollableView);
}

}
