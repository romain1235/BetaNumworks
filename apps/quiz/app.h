#ifndef QUIZ_APP_H
#define QUIZ_APP_H

#include <escher.h>
#include "main_controller.h"

namespace Quiz {

/* Quiz::App
 * ---------
 * Minimal App subclass following the Escher conventions used in the repo.
 * - `Descriptor` provides app metadata (name, upperName, icon).
 * - `Snapshot` is the persistent snapshot that can `unpack` an App in a
 *   given container buffer (used by the apps container to restore apps).
 * - The App owns a `StackViewController` and a `MainController` used as the
 *   UI root. The `StackViewController` is passed to the base `::App` so the
 *   framework knows which view controller to display for this app.
 */
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
	static App * app() { return static_cast<App *>(Container::activeApp()); }
private:
	App(Snapshot * snapshot);
	// Controller that provides the menu UI
	MainController m_mainController;
	// StackViewController used as the root view controller for this App
	StackViewController m_stackViewController;
};

}

#endif
