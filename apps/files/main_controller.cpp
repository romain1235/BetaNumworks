#include "main_controller.h"
#include <apps/i18n.h>
#include <kandinsky/palette.h>
#include <ion/storage.h>
/* removed debug console includes */
#include "../apps_container.h"
#include <escher.h>

namespace Files {

MainController::MainController(Responder * parentResponder, ::App * app) :
  ViewController(parentResponder),
  m_app(app),
  m_selectableTableView(this),
  m_parameterController(parentResponder)
{
  for (int i = 0; i < k_maxNumberOfCells; i++) {
    m_cells[i].setParentResponder(&m_selectableTableView);
    m_cells[i].textField()->setDelegates(nullptr, this);
  }
}

View * MainController::view() {
  return &m_selectableTableView;
}

void MainController::didBecomeFirstResponder() {
  if (selectedRow() < 0) {
    selectCellAtLocation(0, 0);
  }
  Container::activeApp()->setFirstResponder(&m_selectableTableView);
}

bool MainController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    int row = selectedRow();
    if (row < 0) return false;
    // find corresponding record
    int nb = Ion::Storage::sharedStorage()->numberOfRecords();
    if (nb == 0) return false;
    if (row >= nb) return false;
    Ion::Storage::Record r = Ion::Storage::sharedStorage()->recordAtIndex(row);
    StackViewController * stack = static_cast<StackViewController *>(parentResponder());
    m_parameterController.setRecord(r);
    stack->push(&m_parameterController);
    return true;
  }
  return false;
}

int MainController::numberOfRows() const {
  return m_numberOfRows;
}

KDCoordinate MainController::rowHeight(int j) {
  return Metric::ParameterCellHeight;
}

KDCoordinate MainController::cumulatedHeightFromIndex(int j) {
  return j*rowHeight(0);
}

int MainController::indexFromCumulatedHeight(KDCoordinate offsetY) {
  return offsetY/rowHeight(0);
}

HighlightCell * MainController::reusableCell(int index, int type) {
  assert(index >= 0);
  assert(index < k_maxNumberOfCells);
  return &m_cells[index];
}

int MainController::reusableCellCount(int type) {
  return k_maxNumberOfCells;
}

int MainController::typeAtLocation(int i, int j) {
  return 0;
}

void MainController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  FileNameCell * myTextCell = (FileNameCell *)cell;
  myTextCell->setHighlighted(myTextCell->isHighlighted());
  int nb = Ion::Storage::sharedStorage()->numberOfRecords();
  if (nb == 0) {
    myTextCell->textField()->setText(I18n::translate(I18n::Message::FilesEmpty));
    myTextCell->textField()->setTextColor(Palette::Red);
    return;
  }
  if (index < nb) {
    Ion::Storage::Record r = Ion::Storage::sharedStorage()->recordAtIndex(index);
    const char * name = r.fullName();
    myTextCell->textField()->setText(name != nullptr ? name : "");
    myTextCell->textField()->setTextColor(Palette::PrimaryText);
  } else {
    myTextCell->textField()->setText("");
  }
}

void MainController::viewWillAppear() {
  int nb = Ion::Storage::sharedStorage()->numberOfRecords();
  if (nb == 0) {
    m_numberOfRows = 1; // show FilesEmpty
  } else {
    m_numberOfRows = nb;
  }
  m_selectableTableView.reloadData();
}

void MainController::renameSelectedFile() {
  assert(selectedRow() >= 0);
  int row = selectedRow();
  AppsContainer::sharedAppsContainer()->setShiftAlphaStatus(Ion::Events::ShiftAlphaStatus::AlphaLock);
  m_selectableTableView.selectCellAtLocation(0, row);
  FileNameCell * myCell = static_cast<FileNameCell *>(m_selectableTableView.selectedCell());
  Container::activeApp()->setFirstResponder(myCell);
  myCell->setHighlighted(false);
  TextField * tf = myCell->textField();
  const char * previousText = tf->text();
  tf->setEditing(true);
  tf->setText(previousText);
  tf->setCursorLocation(tf->text() + strlen(previousText));
}

bool MainController::textFieldShouldFinishEditing(TextField * textField, Ion::Events::Event event) {
  return event == Ion::Events::OK || event == Ion::Events::EXE
    || event == Ion::Events::Down || event == Ion::Events::Up;
}

bool MainController::textFieldDidFinishEditing(TextField * textField, const char * text, Ion::Events::Event event) {
  int currentRow = m_selectableTableView.selectedRow();
  if (currentRow < 0) return true;
  int nb = Ion::Storage::sharedStorage()->numberOfRecords();
  if (currentRow >= nb) return true;
  Ion::Storage::Record r = Ion::Storage::sharedStorage()->recordAtIndex(currentRow);
  Ion::Storage::Record::ErrorStatus err = r.setName(text);
  if (err == Ion::Storage::Record::ErrorStatus::None) {
    m_selectableTableView.reloadData();
    textField->setText(text);
    if (event == Ion::Events::Down && currentRow < numberOfRows() - 1) {
      m_selectableTableView.selectCellAtLocation(m_selectableTableView.selectedColumn(), currentRow + 1);
    } else if (event == Ion::Events::Up && currentRow > 0) {
      m_selectableTableView.selectCellAtLocation(m_selectableTableView.selectedColumn(), currentRow - 1);
    }
    m_selectableTableView.selectedCell()->setHighlighted(true);
    Container::activeApp()->setFirstResponder(&m_selectableTableView);
    return true;
  } else if (err == Ion::Storage::Record::ErrorStatus::NameTaken) {
    Container::activeApp()->displayWarning(I18n::Message::NameTaken);
  } else if (err == Ion::Storage::Record::ErrorStatus::NonCompliantName) {
    Container::activeApp()->displayWarning(I18n::Message::AllowedCharactersaz09, I18n::Message::NameCannotStartWithNumber);
  } else {
    Container::activeApp()->displayWarning(I18n::Message::NameTooLong);
  }
  // On error, keep focus on text field so user can correct
  return false;
}

bool MainController::textFieldDidAbortEditing(TextField * textField) {
  // Restore table focus
  Container::activeApp()->setFirstResponder(&m_selectableTableView);
  return true;
}

bool MainController::textFieldDidReceiveEvent(TextField * textField, Ion::Events::Event event) {
  // Use default TextField behavior: let TextField::privateHandleEvent handle
  // Back (abort) and Backspace (delete) while editing.
  return false;
}

}
