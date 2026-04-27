#include "console_line_cell.h"
#include "console_controller.h"
#include <kandinsky/point.h>
#include <kandinsky/coordinate.h>
#include <apps/i18n.h>
#include <apps/global_preferences.h>

namespace {

bool IsColorPrefix(const char * p) {
  return p[0] == '\x1b' && p[1] == '[' && p[2] == 'C';
}

const char * SkipColorHeader(const char * p) {
  const char * h = p + 3;
  while (*h != 0 && *h != ';') {
    h++;
  }
  return *h == ';' ? h + 1 : h;
}

const char * FindColorSuffix(const char * p) {
  while (*p != '\0') {
    if (p[0] == '\x1b' && p[1] == '[' && p[2] == '0' && p[3] == 'm') {
      return p;
    }
    p++;
  }
  return p;
}

void StripColorSequences(const char * text, char * buffer, size_t bufferSize) {
  if (bufferSize == 0) {
    return;
  }
  size_t out = 0;
  for (const char * p = text; *p != 0 && out + 1 < bufferSize; ) {
    if (IsColorPrefix(p)) {
      const char * segmentStart = SkipColorHeader(p);
      const char * segmentEnd = FindColorSuffix(segmentStart);
      while (segmentStart < segmentEnd && out + 1 < bufferSize) {
        buffer[out++] = *segmentStart++;
      }
      p = *segmentEnd == '\x1b' ? segmentEnd + 4 : segmentEnd;
      continue;
    }
    buffer[out++] = *p++;
  }
  buffer[out] = 0;
}

void DrawConsoleText(KDContext * ctx, const char * text, const KDFont * font, KDColor defaultColor, KDColor background) {
  KDCoordinate x = 0;
  for (const char * p = text; *p != '\0'; ) {
    if (IsColorPrefix(p)) {
      const char * h = p + 3;
      int r = 0, g = 0, b = 0;
      while (*h >= '0' && *h <= '9') { r = r * 10 + (*h - '0'); h++; }
      if (*h == ',') { h++; }
      while (*h >= '0' && *h <= '9') { g = g * 10 + (*h - '0'); h++; }
      if (*h == ',') { h++; }
      while (*h >= '0' && *h <= '9') { b = b * 10 + (*h - '0'); h++; }
      if (*h == ';') { h++; }
      const char * segStart = h;
      const char * segEnd = FindColorSuffix(segStart);
      int segLen = segEnd - segStart;
      if (segLen > 0) {
        KDPoint point(x, 0);
        if (segLen < 256) {
          char buf[256];
          memcpy(buf, segStart, segLen);
          buf[segLen] = '\0';
          KDSize s = font->stringSize(buf);
          ctx->drawString(buf, point, font, KDColor::RGB888(r, g, b), background);
          x += s.width();
        } else {
          char save = segStart[segLen];
          ((char *)segStart)[segLen] = '\0';
          KDSize s = font->stringSize(segStart);
          ctx->drawString(segStart, point, font, KDColor::RGB888(r, g, b), background);
          ((char *)segStart)[segLen] = save;
          x += s.width();
        }
      }
      p = *segEnd == '\x1b' ? segEnd + 4 : segEnd;
    } else {
      KDPoint point(x, 0);
      KDSize s = font->stringSize(p);
      ctx->drawString(p, point, font, defaultColor, background);
      x += s.width();
      break;
    }
  }
}

KDSize ConsoleTextSize(const char * text, const KDFont * font) {
  KDCoordinate width = 0;
  for (const char * p = text; *p != '\0'; ) {
    if (IsColorPrefix(p)) {
      const char * segStart = SkipColorHeader(p);
      const char * segEnd = FindColorSuffix(segStart);
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
      p = *segEnd == '\x1b' ? segEnd + 4 : segEnd;
    } else {
      width += font->stringSize(p).width();
      break;
    }
  }
  return KDSize(width, font->glyphSize().height());
}

}

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
  const KDFont * font = GlobalPreferences::sharedGlobalPreferences()->font();
  KDColor defaultColor = textColor(m_line);
  DrawConsoleText(ctx, m_line->text(), font, defaultColor, background);
}

KDSize ConsoleLineCell::ScrollableConsoleLineView::ConsoleLineView::minimalSizeForOptimalDisplay() const {
  const KDFont * font = GlobalPreferences::sharedGlobalPreferences()->font();
  return ConsoleTextSize(m_line->text(), font);
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
  StripColorSequences(m_line.text(), m_sanitizedTextBuffer, sizeof(m_sanitizedTextBuffer));
  reloadCell();
}

const char * ConsoleLineCell::text() const {
  return m_sanitizedTextBuffer;
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
