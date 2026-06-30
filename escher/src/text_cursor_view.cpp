#include <escher/text_cursor_view.h>
#include <escher/cursor_blink_timer.h>
#include <escher/palette.h>

bool TextCursorView::s_blinkVisible = true;

TextCursorView::TextCursorView() :
  View(),
  m_blinking(false),
  m_registered(false),
  m_nextBlinkingCursor(nullptr)
{
}

TextCursorView::TextCursorView(TextCursorView&& other) :
  View(static_cast<View&&>(other)),
  m_blinking(other.m_blinking),
  m_registered(other.m_registered),
  m_nextBlinkingCursor(other.m_nextBlinkingCursor)
{
  /* Cursor views are only moved at construction time, before any blinking has
   * started. We make sure the moved-from cursor won't try to unregister itself
   * (which would leave a dangling pointer pointing to it in the timer's list). */
  if (other.m_registered) {
    CursorBlinkTimer::sharedTimer()->unregisterCursor(&other);
    other.m_blinking = false;
  }
  m_registered = false;
  m_nextBlinkingCursor = nullptr;
  if (m_blinking) {
    CursorBlinkTimer::sharedTimer()->registerCursor(this);
  }
}

TextCursorView::~TextCursorView() {
  if (m_registered) {
    CursorBlinkTimer::sharedTimer()->unregisterCursor(this);
  }
}

void TextCursorView::setBlinking(bool blink) {
  if (m_blinking == blink) {
    return;
  }
  m_blinking = blink;
  if (blink) {
    s_blinkVisible = true;
    CursorBlinkTimer::sharedTimer()->registerCursor(this);
  } else {
    CursorBlinkTimer::sharedTimer()->unregisterCursor(this);
  }
  markRectAsDirty(bounds());
}

void TextCursorView::setBlinkVisible(bool visible) {
  s_blinkVisible = visible;
}

bool TextCursorView::blinkVisible() {
  return s_blinkVisible;
}

void TextCursorView::redrawBlink() {
  /* Mark the area of our superview that we cover as dirty, so that the text and
   * background underneath the cursor are redrawn (which erases the cursor) and
   * the cursor is then drawn on top in its current blink state. */
  if (m_superview != nullptr) {
    m_superview->markRectAsDirty(m_frame);
  } else {
    markRectAsDirty(bounds());
  }
}

void TextCursorView::drawRect(KDContext * ctx, KDRect rect) const {
  if (m_blinking && !s_blinkVisible) {
    return;
  }
  KDCoordinate height = bounds().height();
  ctx->fillRect(KDRect(0, 0, k_width, height), Palette::PrimaryText);
}

KDSize TextCursorView::minimalSizeForOptimalDisplay() const {
  return KDSize(k_width, 0);
}
