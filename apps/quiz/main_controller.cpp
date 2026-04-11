#include "main_controller.h"
#include "../apps_container.h"

/*
 * main_controller.cpp
 * -------------------
 * Minimal, well-commented example of an Escher menu controller.
 * Purpose:
 *  - Show how a StackViewController wraps an inner list controller.
 *  - Demonstrate the reusable-cell pattern and how selections are handled.
 *
 * Patterns to notice:
 *  - InnerListController: view controller holding the SelectableTableView.
 *  - Reusable cells: allocate a small array of cell objects reused by the
 *    table to avoid dynamic allocation when scrolling.
 *  - App interaction: call Container::activeApp()->displayWarning(...) to
 *    show a simple modal; in production you'd push other controllers.
 */

namespace Quiz {

// Construct the inner controller that owns the SelectableTableView.
MainController::InnerListController::InnerListController(MainController * dataSource) :
  ViewController(dataSource),
  m_selectableTableView(this, dataSource, dataSource, dataSource)
{
  // Table view styling: no extra margins and no scroll decorator for compactness
  m_selectableTableView.setMargins(0);
  m_selectableTableView.setDecoratorType(ScrollView::Decorator::Type::None);
}

// When the inner controller becomes first responder, reload the table data.
void MainController::InnerListController::didBecomeFirstResponder() {
  m_selectableTableView.reloadData();
}

// MainController constructor: initialize StackViewController with the inner list
MainController::MainController() :
  StackViewController(nullptr, &m_listController, Palette::ToolboxHeaderText, Palette::ToolboxHeaderBackground, Palette::ToolboxHeaderBorder),
  m_listController(this)
{
  // Simple menu entries using i18n messages
  m_menuMessages[0] = I18n::Message::QuizApp;
  m_menuMessages[1] = I18n::Message::QuizAppCapital;
  m_menuMessages[2] = I18n::Message::About;
  m_menuMessages[3] = I18n::Message::QuizApp;
}

// Handle OK/EXE: fetch the selected row and perform the demo action.
bool MainController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    int row = m_listController.selectableTableView()->selectedRow();
    if (row >= 0 && row < k_numberOfMenuRows) {
      // For demo we show a small modal warning with the i18n message.
      // Real apps usually push another ViewController on the stack.
      Container::activeApp()->displayWarning(m_menuMessages[row]);
      return true;
    }
  }
  return false;
}

// Ensure the list controller receives events.
void MainController::didBecomeFirstResponder() {
  Container::activeApp()->setFirstResponder(&m_listController);
}

// Data-source size: number of rows in the menu
int MainController::numberOfRows() const {
  return k_numberOfMenuRows;
}

// Row height uses the metric constants defined in the UI library
KDCoordinate MainController::rowHeight(int j) {
  return Metric::ToolboxRowHeight;
}

// Return a pointer to a reusable cell instance (cycle through the pool)
HighlightCell * MainController::reusableCell(int index, int type) {
  return &m_cells[index % k_maxNumberOfDisplayedRows];
}

int MainController::reusableCellCount(int type) {
  return k_maxNumberOfDisplayedRows;
}

// When the table asks to display a cell, set its message (label)
void MainController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  MessageTableCellWithChevron<> * myCell = static_cast<MessageTableCellWithChevron<> *>(cell);
  myCell->setMessage(m_menuMessages[index]);
}

} // namespace Quiz

