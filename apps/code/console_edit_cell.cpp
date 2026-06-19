#include "console_edit_cell.h"
#include "console_controller.h"
#include <escher/app.h>
#include <apps/i18n.h>
#include <apps/global_preferences.h>
#include <assert.h>
#include <algorithm>

namespace {

bool IsColorPrefix(const char * p) {
  return p != nullptr && p[0] == '\x1b' && p[1] == '[' && p[2] == 'C';
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

void DrawConsoleText(KDContext * ctx, const char * text, const KDFont * font, KDColor defaultColor, KDColor background) {
  KDCoordinate x = 0;
  if (text == nullptr) {
    return;
  }
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
      const char * segEnd = p;
      while (*segEnd != '\0' && !IsColorPrefix(segEnd)) {
        segEnd++;
      }
      int segLen = segEnd - p;
      if (segLen > 0) {
        KDPoint point(x, 0);
        if (segLen < 256) {
          char buf[256];
          memcpy(buf, p, segLen);
          buf[segLen] = '\0';
          KDSize s = font->stringSize(buf);
          ctx->drawString(buf, point, font, defaultColor, background);
          x += s.width();
        } else {
          char save = p[segLen];
          ((char *)p)[segLen] = '\0';
          KDSize s = font->stringSize(p);
          ctx->drawString(p, point, font, defaultColor, background);
          ((char *)p)[segLen] = save;
          x += s.width();
        }
      }
      p = segEnd;
    }
  }
}

KDSize ConsoleTextSize(const char * text, const KDFont * font) {
  if (text == nullptr) {
    return KDSizeZero;
  }
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
      const char * segEnd = p;
      while (*segEnd != '\0' && !IsColorPrefix(segEnd)) {
        segEnd++;
      }
      int segLen = segEnd - p;
      if (segLen > 0) {
        if (segLen < 256) {
          char buf[256];
          memcpy(buf, p, segLen);
          buf[segLen] = '\0';
          width += font->stringSize(buf).width();
        } else {
          char save = p[segLen];
          ((char *)p)[segLen] = '\0';
          width += font->stringSize(p).width();
          ((char *)p)[segLen] = save;
        }
      }
      p = segEnd;
    }
  }
  return KDSize(width, font->glyphSize().height());
}

}

namespace Code {

ConsoleEditCell::PromptTextView::PromptTextView() :
  View(),
  m_text(nullptr)
{
}

void ConsoleEditCell::PromptTextView::setText(const char * text) {
  m_text = text;
}

KDSize ConsoleEditCell::PromptTextView::minimalSizeForOptimalDisplay() const {
  return ConsoleTextSize(m_text, GlobalPreferences::sharedGlobalPreferences()->font());
}

void ConsoleEditCell::PromptTextView::drawRect(KDContext * ctx, KDRect rect) const {
  KDColor background = Palette::CodeBackground;
  ctx->fillRect(bounds(), background);
  DrawConsoleText(ctx, m_text, GlobalPreferences::sharedGlobalPreferences()->font(), Palette::CodeText, background);
}

ConsoleEditCell::ConsoleEditCell(Responder * parentResponder, InputEventHandlerDelegate * inputEventHandlerDelegate, TextFieldDelegate * delegate) :
  HighlightCell(),
  Responder(parentResponder),
  m_promptView(),
  m_textField(this, nullptr, TextField::maxBufferSize(), TextField::maxBufferSize(), inputEventHandlerDelegate, delegate, GlobalPreferences::sharedGlobalPreferences()->font())
{
}

int ConsoleEditCell::numberOfSubviews() const {
  return 2;
}

View * ConsoleEditCell::subviewAtIndex(int index) {
  assert(index == 0 || index ==1);
  if (index == 0) {
   return &m_promptView;
  } else {
   return &m_textField;
  }
}

void ConsoleEditCell::layoutSubviews(bool force) {
  KDSize promptSize = m_promptView.minimalSizeForOptimalDisplay();
  m_promptView.setFrame(KDRect(KDPointZero, promptSize.width(), bounds().height()), force);
  m_textField.setFrame(KDRect(KDPoint(promptSize.width(), KDCoordinate(0)), bounds().width() - promptSize.width(), bounds().height()), force);
}

void ConsoleEditCell::didBecomeFirstResponder() {
  Container::activeApp()->setFirstResponder(&m_textField);
}

void ConsoleEditCell::setEditing(bool isEditing) {
  m_textField.setEditing(isEditing);
}

void ConsoleEditCell::setText(const char * text) {
  m_textField.setText(text);
}

void ConsoleEditCell::setPrompt(const char * prompt) {
  m_promptView.setText(prompt);
  layoutSubviews();
}

bool ConsoleEditCell::insertText(const char * text) {
  return m_textField.handleEventWithText(text);
}

void ConsoleEditCell::clearAndReduceSize() {
  setText("");
  size_t previousBufferSize = m_textField.draftTextBufferSize();
  assert(previousBufferSize > 1);
  m_textField.setDraftTextBufferSize(previousBufferSize - 1);
}

const char * ConsoleEditCell::shiftCurrentTextAndClear() {
  size_t previousBufferSize = m_textField.draftTextBufferSize();
  m_textField.setDraftTextBufferSize(previousBufferSize + 1);
  char * textFieldBuffer = const_cast<char *>(m_textField.text());
  char * newTextPosition = textFieldBuffer + 1;
  assert(previousBufferSize > 0);
  size_t copyLength = std::min(previousBufferSize - 1, strlen(textFieldBuffer));
  memmove(newTextPosition, textFieldBuffer, copyLength);
  newTextPosition[copyLength] = 0;
  textFieldBuffer[0] = 0;
  return newTextPosition;
}

}
