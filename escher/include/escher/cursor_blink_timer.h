#ifndef ESCHER_CURSOR_BLINK_TIMER_H
#define ESCHER_CURSOR_BLINK_TIMER_H

#include <escher/timer.h>

class TextCursorView;

class CursorBlinkTimer : public Timer {
public:
  static CursorBlinkTimer * sharedTimer();
  void registerCursor(TextCursorView * cursor);
  void unregisterCursor(TextCursorView * cursor);
  void resetBlinkPhase();
private:
  CursorBlinkTimer();
  bool fire() override;
  TextCursorView * m_firstCursor;
};

#endif
