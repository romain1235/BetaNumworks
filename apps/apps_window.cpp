#include "apps_window.h"
#include <escher/metric.h>
extern "C" {
#include <assert.h>
}
#include <ion/rtc.h>

// Forward kandinsky fullscreen accessor (C linkage)
extern "C" bool modkandinsky_is_fullscreen(void);

AppsWindow::AppsWindow() :
  Window(),
  m_titleBarView(),
  m_hideTitleBarView(false)
{
}

void AppsWindow::setTitle(I18n::Message title) {
  m_titleBarView.setTitle(title);
}

bool AppsWindow::updateBatteryLevel() {
  return m_titleBarView.setChargeState(Ion::Battery::level());
}

bool AppsWindow::updateClock() {
  Ion::RTC::DateTime dateTime = Ion::RTC::dateTime();
  return m_titleBarView.setClock(dateTime.tm_hour, dateTime.tm_min, Ion::RTC::mode() != Ion::RTC::Mode::Disabled);
}

bool AppsWindow::updateIsChargingState() {
  return m_titleBarView.setIsCharging(Ion::Battery::isCharging());
}

bool AppsWindow::updatePluggedState() {
  return m_titleBarView.setIsPlugged(Ion::USB::isPlugged());
}

void AppsWindow::refreshPreferences() {
  m_titleBarView.refreshPreferences();
}

void AppsWindow::reloadTitleBarView() {
  m_titleBarView.reload();
}

bool AppsWindow::updateAlphaLock() {
  return m_titleBarView.setShiftAlphaLockStatus(Ion::Events::shiftAlphaStatus());
}

void AppsWindow::hideTitleBarView(bool hide) {
  if (m_hideTitleBarView != hide) {
    m_hideTitleBarView = hide;
    layoutSubviews();
  }
}

int AppsWindow::numberOfSubviews() const {
  if (modkandinsky_is_fullscreen()) {
    return (m_contentView == nullptr ? 0 : 1);
  }
  return (m_contentView == nullptr ? 1 : 2);
}

View * AppsWindow::subviewAtIndex(int index) {
  if (modkandinsky_is_fullscreen()) {
    assert(m_contentView != nullptr && index == 0);
    return m_contentView;
  }
  if (index == 0) {
    return &m_titleBarView;
  }
  assert(m_contentView != nullptr && index == 1);
  return m_contentView;
}

void AppsWindow::layoutSubviews(bool force) {
  KDCoordinate titleHeight = (m_hideTitleBarView || modkandinsky_is_fullscreen()) ? 0 : Metric::TitleBarHeight;
  m_titleBarView.setFrame(KDRect(0, 0, bounds().width(), titleHeight), force);
  if (m_contentView != nullptr) {
    m_contentView->setFrame(KDRect(0, titleHeight, bounds().width(), bounds().height()-titleHeight), force);
  }
}

#if ESCHER_VIEW_LOGGING
const char * AppsWindow::className() const {
  return "Window";
}
#endif
