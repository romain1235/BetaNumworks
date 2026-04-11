#include "app.h"
#include <apps/i18n.h>
#include <apps/files/files_icon.h>

namespace Files {

I18n::Message App::Descriptor::name() {
  return I18n::Message::FilesApp;
}

I18n::Message App::Descriptor::upperName() {
  return I18n::Message::FilesAppCapital;
}

const Image * App::Descriptor::icon() {
  return ImageStore::FilesIcon;
}

App * App::Snapshot::unpack(Container * container) {
  return new (container->currentAppBuffer()) App(this);
}

App::Descriptor * App::Snapshot::descriptor() {
  static Descriptor descriptor;
  return &descriptor;
}

App::App(Snapshot * snapshot) :
  ::App(snapshot, &m_stackViewController),
  m_mainController(&m_stackViewController, this),
  m_stackViewController(&m_modalViewController, &m_mainController)
{
}

void App::didBecomeActive(Window * window) {
  ::App::didBecomeActive(window);
  m_window = window;
}

void App::redraw() {
  if (m_window) m_window->redraw(true);
}

}
