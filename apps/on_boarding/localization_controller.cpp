#include "localization_controller.h"
#include "app.h"
#include <algorithm>
#include <apps/apps_container.h>
#include <apps/global_preferences.h>
#include <apps/theme/theme_loader.h>

namespace OnBoarding {

int LocalizationController::indexOfCellToSelectOnReset() const {
  return mode() == Mode::Language ?
    Shared::LocalizationController::indexOfCellToSelectOnReset() :
    IndexOfCountry(I18n::DefaultCountryForLanguage[static_cast<uint8_t>(GlobalPreferences::sharedGlobalPreferences()->language())]);
}

bool LocalizationController::handleEvent(Ion::Events::Event event) {
  if (Shared::LocalizationController::handleEvent(event)) {
    if (mode() == Mode::Language) {
      setMode(Mode::Country);
      viewWillAppear();
    } else {
      assert(mode() == Mode::Country);
      // If there are .theme files on flash, show the theme picker before going home
      if (ThemeLoader::numberOfThemeFiles() > 0) {
        static_cast<App *>(Container::activeApp())->showThemePicker();
      } else {
        AppsContainer * appsContainer = AppsContainer::sharedAppsContainer();
        if (appsContainer->promptController()) {
          Container::activeApp()->displayModalViewController(appsContainer->promptController(), 0.5f, 0.5f);
        } else {
          appsContainer->switchTo(appsContainer->appSnapshotAtIndex(0));
        }
      }
    }
    return true;
  }
  if (event == Ion::Events::Back) {
    if (mode() == Mode::Country) {
      setMode(Mode::Language);
      viewWillAppear();
    }
    return true;
  }
  return false;
}

}
