#ifndef FILES_APP_H
#define FILES_APP_H

#include <escher.h>
#include "main_controller.h"

namespace Files {

class App : public ::App {
public:
  class Descriptor : public ::App::Descriptor {
  public:
    I18n::Message name() override;
    I18n::Message upperName() override;
    const Image * icon() override;
  };
  class Snapshot : public ::App::Snapshot {
  public:
    App * unpack(Container * container) override;
    Descriptor * descriptor() override;
  };
  void redraw();
  virtual void didBecomeActive(Window * window);
  static App * app() { return static_cast<App *>(Container::activeApp()); }
  Snapshot * snapshot() const { return static_cast<Snapshot *>(::App::snapshot()); }
private:
  App(Snapshot * snapshot);
  MainController m_mainController;
  StackViewController m_stackViewController;
  Window * m_window;
};

}

#endif
