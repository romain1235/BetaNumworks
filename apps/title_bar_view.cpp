#include "title_bar_view.h"
#include "exam_icon.h"
#include "global_preferences.h"
extern "C" {
#include <assert.h>
}

using namespace Poincare;

TitleBarView::TitleBarView() :
  View(),
  m_titleView(KDFont::SmallFont, I18n::Message::Default, 0.5f, 0.5f, Palette::ToolbarText, Palette::Toolbar),
  m_preferenceView(KDFont::SmallFont, 1.0f, 0.5, Palette::ToolbarText, Palette::Toolbar),
  m_batteryPercentView(KDFont::SmallFont, 1.0f, 0.5f, Palette::ToolbarText, Palette::Toolbar),
  m_clockView(KDFont::SmallFont, 0.5f, 0.5f, Palette::ToolbarText, Palette::Toolbar),
  m_hours(-1),
  m_mins(-1),
  m_clockEnabled(Ion::RTC::mode() != Ion::RTC::Mode::Disabled),
  m_batterySampleIndex(0),
  m_batterySampleCount(0),
  m_lastBatteryPercent(-1)
{
  setClock(Ion::RTC::dateTime().tm_hour, Ion::RTC::dateTime().tm_min, m_clockEnabled);
  m_examModeIconView.setImage(ImageStore::ExamIcon);
  // Initialize battery percent text (use smoothing)
  for (int i = 0; i < k_batterySamples; i++) {
    m_batteryVoltageSamples[i] = 0.0f;
  }
  updateBatteryPercent(true);
}

void TitleBarView::updateBatteryPercent(bool force) {
  // Push new sample
  const float v = Ion::Battery::voltage();
  m_batteryVoltageSamples[m_batterySampleIndex] = v;
  m_batterySampleIndex = (m_batterySampleIndex + 1) % k_batterySamples;
  if (m_batterySampleCount < k_batterySamples) {
    m_batterySampleCount++;
  }
  // Compute average
  float sum = 0.0f;
  for (int i = 0; i < m_batterySampleCount; i++) {
    sum += m_batteryVoltageSamples[i];
  }
  const float avg = sum / (float)m_batterySampleCount;
  int pct = (int)((avg - 3.3f) / (4.2f - 3.3f) * 100.0f + 0.5f);
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  // Keep percent based on averaged voltage; do not force 100% from level()
  if (pct == m_lastBatteryPercent && !force) {
    return;
  }
  m_lastBatteryPercent = pct;
  char buf[5] = "";
  if (pct == 100) {
    buf[0] = '1'; buf[1] = '0'; buf[2] = '0'; buf[3] = '%'; buf[4] = '\0';
  } else if (pct >= 10) {
    buf[0] = '0' + (pct/10); buf[1] = '0' + (pct%10); buf[2] = '%'; buf[3] = '\0';
  } else {
    buf[0] = '0' + pct; buf[1] = '%'; buf[2] = '\0';
  }
  m_batteryPercentView.setText(buf);
}

void TitleBarView::drawRect(KDContext * ctx, KDRect rect) const {
  /* As we cheated to layout the title view, we have to fill a very thin
   * rectangle at the top with the background color. */
  ctx->fillRect(KDRect(0, 0, bounds().width(), 2), Palette::Toolbar);
}

void TitleBarView::setTitle(I18n::Message title) {
  m_titleView.setMessage(title);
}

bool TitleBarView::setClock(int hours, int mins, bool enabled) {
  bool changed = m_clockEnabled != enabled;

  if (!enabled) {
    m_clockView.setText("");
    hours = -1;
    mins = -1;
  }
  else if (m_hours != hours || m_mins != mins) {
    char buf[6], *ptr = buf;
    *ptr++ = (hours / 10) + '0';
    *ptr++ = (hours % 10) + '0';
    *ptr++ = ':';
    *ptr++ = (mins / 10) + '0';
    *ptr++ = (mins % 10) + '0';
    *ptr   = '\0';
    m_clockView.setText(buf);

    changed = true;
  }
  if (m_clockEnabled != enabled) {
    layoutSubviews();
    m_clockEnabled = enabled; 
  }

  m_hours = hours;
  m_mins = mins;
  return changed;
}

bool TitleBarView::setChargeState(Ion::Battery::Charge chargeState) {
  bool changed = m_batteryView.setChargeState(chargeState);
  updateBatteryPercent();
  if (changed) {
    layoutSubviews();
  }
  return changed;
}

bool TitleBarView::setIsCharging(bool isCharging) {
  bool changed = m_batteryView.setIsCharging(isCharging);
  updateBatteryPercent();
  if (changed) {
    layoutSubviews();
  }
  return changed;
}

bool TitleBarView::setIsPlugged(bool isPlugged) {
  bool changed = m_batteryView.setIsPlugged(isPlugged);
  updateBatteryPercent();
  if (changed) {
    layoutSubviews();
  }
  return changed;
}

bool TitleBarView::setShiftAlphaLockStatus(Ion::Events::ShiftAlphaStatus status) {
  return m_shiftAlphaLockView.setStatus(status);
}

int TitleBarView::numberOfSubviews() const {
  return 7;
}

View * TitleBarView::subviewAtIndex(int index) {
  if (index == 0) {
    return &m_titleView;
  }
  if (index == 1) {
    return &m_preferenceView;
  }
  if (index == 2) {
    return &m_examModeIconView;
  }
  if (index == 3) {
    return &m_shiftAlphaLockView;
  }
  if (index == 4) {
    return &m_clockView;
  }
  if (index == 5) {
    return &m_batteryPercentView;
  }
  return &m_batteryView;
}

void TitleBarView::layoutSubviews(bool force) {
  /* We here cheat to layout the main title. The application title is written
   * with upper cases. But, as upper letters are on the same baseline as lower
   * letters, they seem to be slightly above when they are perfectly centered
   * (because their glyph never cross the baseline). To avoid this effect, we
   * translate the frame of the title downwards.*/
  m_titleView.setFrame(KDRect(0, 2, bounds().width(), bounds().height()-2), force);
  m_preferenceView.setFrame(KDRect(Metric::TitleBarExternHorizontalMargin, 0, m_preferenceView.minimalSizeForOptimalDisplay().width(), bounds().height()), force);
  KDSize clockSize = m_clockView.minimalSizeForOptimalDisplay();
  m_clockView.setFrame(KDRect(bounds().width() - clockSize.width() - Metric::TitleBarExternHorizontalMargin, (bounds().height()- clockSize.height())/2, clockSize), force);
  if (clockSize.width() != 0) {
    clockSize = KDSize(clockSize.width() + k_alphaRightMargin, clockSize.height());
  }
  KDSize batterySize = m_batteryView.minimalSizeForOptimalDisplay();
  KDCoordinate batteryX = bounds().width() - clockSize.width() - batterySize.width() - Metric::TitleBarExternHorizontalMargin;
  m_batteryView.setFrame(KDRect(batteryX, (bounds().height()- batterySize.height())/2, batterySize), force);
  // Position battery percent view just to the left of the battery (one space)
  KDSize percentSize = m_batteryPercentView.minimalSizeForOptimalDisplay();
  KDCoordinate space = KDFont::SmallFont->glyphSize().width();
  m_batteryPercentView.setFrame(KDRect(batteryX - percentSize.width() - space, (bounds().height()- percentSize.height())/2, percentSize), force);
  if (GlobalPreferences::sharedGlobalPreferences()->isInExamMode()) {
    KDCoordinate examIconX = batteryX - percentSize.width() - space - k_examIconWidth - k_alphaRightMargin;
    m_examModeIconView.setFrame(KDRect(examIconX, (bounds().height() - k_examIconHeight)/2, k_examIconWidth, k_examIconHeight), force);
  } else {
    m_examModeIconView.setFrame(KDRectZero, force);
  }
  // Place the Shift/Alpha indicator to the right of the preference view (sym/deg)
  KDSize prefSize = m_preferenceView.minimalSizeForOptimalDisplay();
  KDCoordinate prefRight = Metric::TitleBarExternHorizontalMargin + prefSize.width();
  KDSize shiftAlphaLockSize = m_shiftAlphaLockView.minimalSizeForOptimalDisplay();
  m_shiftAlphaLockView.setFrame(KDRect(prefRight + k_alphaRightMargin, (bounds().height()- shiftAlphaLockSize.height())/2, shiftAlphaLockSize), force);
}

void TitleBarView::refreshPreferences() {
  constexpr size_t bufferSize = 13;
  char buffer[bufferSize];
  int numberOfChar = 0;
  Preferences * preferences = Preferences::sharedPreferences();
  if (GlobalPreferences::sharedGlobalPreferences()->isInExamModeSymbolic()) {
    // Display "cas" if in exam mode with symbolic computation enabled
      numberOfChar += strlcpy(buffer+numberOfChar, I18n::translate(I18n::Message::Sym), bufferSize - numberOfChar);
      assert(numberOfChar < bufferSize-1);
      assert(UTF8Decoder::CharSizeOfCodePoint('/') == 1);
      buffer[numberOfChar++] = '/';
  }
  assert(numberOfChar <= bufferSize);
  // Note: removed display of Sci/Eng float mode; show only sym/angle_unit
  assert(numberOfChar <= bufferSize);
  {
    // Display the angle unit
    const Preferences::AngleUnit angleUnit = preferences->angleUnit();
    I18n::Message angleMessage = angleUnit == Preferences::AngleUnit::Degree ?
        I18n::Message::Deg :
        (angleUnit == Preferences::AngleUnit::Radian ? I18n::Message::Rad : I18n::Message::Gon);
    numberOfChar += strlcpy(buffer+numberOfChar, I18n::translate(angleMessage), bufferSize - numberOfChar);
    assert(numberOfChar < bufferSize-1);
  }
  
  m_preferenceView.setText(buffer);
  // Layout the exam mode icon if needed
  layoutSubviews();
}

void TitleBarView::reload() {
  // Re-apply current palette colors to all sub-views whose colors were
  // captured by value at construction time.
  m_titleView.setTextColor(Palette::ToolbarText);
  m_titleView.setBackgroundColor(Palette::Toolbar);
  m_preferenceView.setTextColor(Palette::ToolbarText);
  m_preferenceView.setBackgroundColor(Palette::Toolbar);
  m_batteryPercentView.setTextColor(Palette::ToolbarText);
  m_batteryPercentView.setBackgroundColor(Palette::Toolbar);
  m_clockView.setTextColor(Palette::ToolbarText);
  m_clockView.setBackgroundColor(Palette::Toolbar);
  m_shiftAlphaLockView.reload();
  refreshPreferences();
  markRectAsDirty(bounds());
}
