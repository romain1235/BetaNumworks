#ifndef ESCHER_TEXT_CURSOR_VIEW_H
#define ESCHER_TEXT_CURSOR_VIEW_H

#include <escher/view.h>

class CursorBlinkTimer;

class TextCursorView : public View {
  friend class CursorBlinkTimer;
public:
  TextCursorView();
  TextCursorView(TextCursorView&& other);
  ~TextCursorView();
  KDRect frame() const { return m_frame; }
  void drawRect(KDContext * ctx, KDRect rect) const override;
  KDSize minimalSizeForOptimalDisplay() const override;
  void setBlinking(bool blink);
  static void setBlinkVisible(bool visible);
  static bool blinkVisible();
  constexpr static KDCoordinate k_width = 1;
private:
  void redrawBlink();
  bool m_blinking;
  bool m_registered;
  TextCursorView * m_nextBlinkingCursor;
  static bool s_blinkVisible;
};

#endif
