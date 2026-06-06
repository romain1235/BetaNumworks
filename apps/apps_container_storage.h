#ifndef APPS_CONTAINER_STORAGE_H
#define APPS_CONTAINER_STORAGE_H

#include "apps_container.h"

#ifndef APPS_CONTAINER_SNAPSHOT_DECLARATIONS
#error Missing snapshot declarations
#endif

class AppsContainerStorage : public AppsContainer {
public:
  AppsContainerStorage();
  int numberOfApps() override;
  App::Snapshot * appSnapshotAtIndex(int index) override;
  int appIndexFromSnapshot(App::Snapshot * snapshot) override;
  void * currentAppBuffer() override { return &m_apps; };
private:
  union Apps {
  public:
    /* Enforce a trivial constructor and destructor that just leave the memory
     * unmodified. This way, m_apps can be trivially destructed. */
    Apps() {};
    ~Apps() {};
  private:
    APPS_CONTAINER_APPS_DECLARATION
    Home::App m_homeApp;
    OnBoarding::App m_onBoardingApp;
    HardwareTest::App m_hardwareTestApp;
    USB::App m_usbApp;
  };
  Apps m_apps;
  // static_assert(sizeof(Apps) != 1); // Uncomment this line to log the Apps union size
  #ifdef CODE_APP_H
  static_assert(sizeof(Apps) == sizeof(Code::App), "Code should be the biggest app. Increase the Python heap in apps/code/app.h to match available memory to avoid wasting RAM");
  #endif

  // TODO: Assert code app is almost the same size as Home app
  APPS_CONTAINER_SNAPSHOT_DECLARATIONS
};

#endif
