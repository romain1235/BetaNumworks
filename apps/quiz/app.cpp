#include "app.h"
#include <apps/i18n.h>
#include <apps/quiz/quiz_icon.h>

/* Implementation notes:
 * - Descriptor methods return I18n message identifiers used by the UI.
 * - Snapshot::unpack constructs the App in the buffer provided by the
 *   container (placement new). This is how the apps container restores
 *   running apps without separate heap allocations.
 * - The App constructor wires the `StackViewController` and the
 *   `MainController` together. Order matters in the initialization list:
 *     1) Pass the address of `m_stackViewController` to the base `::App`
 *        so the modal controller inside `::App` uses it as root view.
 *     2) Initialize `m_mainController` (no-arg default construction).
 *     3) Initialize `m_stackViewController` with the modal controller as
 *        parent and `m_mainController` as its root ViewController.
 */

namespace Quiz {

I18n::Message App::Descriptor::name() {
	return I18n::Message::QuizApp;
}

I18n::Message App::Descriptor::upperName() {
	return I18n::Message::QuizAppCapital;
}

const Image * App::Descriptor::icon() {
	return ImageStore::QuizIcon;
}

App * App::Snapshot::unpack(Container * container) {
	// Placement-new the App inside the container's app buffer
	return new (container->currentAppBuffer()) App(this);
}

App::Descriptor * App::Snapshot::descriptor() {
	static Descriptor descriptor;
	return &descriptor;
}

App::App(Snapshot * snapshot) :
	// Provide the StackViewController to the base App so it becomes the
	// application's root view controller.
	::App(snapshot, &m_stackViewController),
	// Default-construct the main controller
	m_mainController(),
	// Initialize the stack with the modal view controller parent and the
	// main controller as the initial (root) view controller.
	m_stackViewController(&m_modalViewController, &m_mainController)
{
}

}
