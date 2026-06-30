#include <escher/cursor_blink_timer.h>
#include <escher/text_cursor_view.h>
#include <escher/timer_manager.h>

CursorBlinkTimer::CursorBlinkTimer() :
  Timer(2),
  m_firstCursor(nullptr)
{
}

CursorBlinkTimer * CursorBlinkTimer::sharedTimer() {
  static CursorBlinkTimer timer;
  return &timer;
}

void CursorBlinkTimer::registerCursor(TextCursorView * cursor) {
  if (cursor->m_registered) {
    return;
  }
  bool wasEmpty = m_firstCursor == nullptr;
  cursor->m_nextBlinkingCursor = m_firstCursor;
  cursor->m_registered = true;
  m_firstCursor = cursor;
  /* Only register the timer in the run loop once, when the first cursor starts
   * blinking. Adding it several times would create a cycle in the run loop's
   * timer list. */
  if (wasEmpty) {
    TimerManager::AddTimer(this);
  }
}

void CursorBlinkTimer::unregisterCursor(TextCursorView * cursor) {
  if (!cursor->m_registered) {
    return;
  }
  TextCursorView ** previous = &m_firstCursor;
  TextCursorView * current = m_firstCursor;
  while (current != nullptr) {
    if (current == cursor) {
      *previous = current->m_nextBlinkingCursor;
      cursor->m_nextBlinkingCursor = nullptr;
      cursor->m_registered = false;
      if (m_firstCursor == nullptr) {
        TimerManager::RemoveTimer(this);
      }
      return;
    }
    previous = &current->m_nextBlinkingCursor;
    current = current->m_nextBlinkingCursor;
  }
}

void CursorBlinkTimer::resetBlinkPhase() {
  TextCursorView::setBlinkVisible(true);
  reset();
}

bool CursorBlinkTimer::fire() {
  TextCursorView::setBlinkVisible(!TextCursorView::blinkVisible());
  TextCursorView * cursor = m_firstCursor;
  while (cursor != nullptr) {
    cursor->redrawBlink();
    cursor = cursor->m_nextBlinkingCursor;
  }
  return true;
}
