#include "files_parameter_controller.h"
#include "main_controller.h"
#include "delete_pop_up_controller.h"
#include <apps/i18n.h>
#include "../apps_container.h"
// avoid <cstdio> portability issues on target toolchain

using namespace Files;

FilesParameterController::FilesParameterController(Responder * parentResponder) :
  ViewController(parentResponder),
  m_selectableTableView(this),
  m_deleteCell(I18n::Message::Delete),
  m_renameCell(I18n::Message::Rename),
  m_confirmPopUpController(Invocation([](void * ctx, void * sender) {
    FilesParameterController * self = (FilesParameterController *)ctx;
    // destroy the record, dismiss popup, pop parameter controller and refresh list
    StackViewController * stack = static_cast<StackViewController *>(self->parentResponder());
    if (!self->m_record.isNull()) {
      self->m_record.destroy();
    }
    Container::activeApp()->dismissModalViewController();
    stack->pop();
    ViewController * top = stack->topViewController();
    if (top) {
      top->viewWillAppear();
    }
    return true;
  }, this))
{
}

const char * FilesParameterController::title() {
  return I18n::translate(I18n::Message::Options);
}

void FilesParameterController::viewWillAppear() {
  ViewController::viewWillAppear();
  m_selectableTableView.reloadData();
  m_selectableTableView.selectCellAtLocation(0, 0);
}

void FilesParameterController::didBecomeFirstResponder() {
  selectCellAtLocation(0, 0);
  Container::activeApp()->setFirstResponder(&m_selectableTableView);
}

HighlightCell * FilesParameterController::reusableCell(int index) {
  assert(index >= 0 && index < k_totalNumberOfCell);
  HighlightCell * cells[] = {&m_renameCell, &m_deleteCell};
  return cells[index];
}

void FilesParameterController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  // nothing special
}

bool FilesParameterController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    int row = m_selectableTableView.selectedRow();
    StackViewController * stack = static_cast<StackViewController *>(parentResponder());
    switch (row) {
      case 0: { // Rename
        // Pop parameter controller and ask main controller to start inline rename
        stack->pop();
        ViewController * top = stack->topViewController();
        if (top) {
          Files::MainController * mc = static_cast<Files::MainController *>(top);
          mc->renameSelectedFile();
        }
        return true;
      }
      case 1: { // Delete
        // Open confirmation pop-up
        Container::activeApp()->displayModalViewController(&m_confirmPopUpController, 0.f, 0.f, Metric::ExamPopUpTopMargin, Metric::PopUpRightMargin, Metric::ExamPopUpBottomMargin, Metric::PopUpLeftMargin);
        return true;
      }
      default:
        return false;
    }
  }
  return false;
}

void FilesParameterController::renameRecord(Ion::Storage::Record record, const char * fullName) {
  if (record.isNull()) return;
  // Parse provided fullName into base and extension without relying on C string helpers
  char base[TextField::maxBufferSize()];
  const char * extension = "";
  // compute length
  size_t len = 0;
  while (fullName[len] != '\0') { len++; }
  // find last dot
  const char * dot = nullptr;
  for (int i = (int)len - 1; i >= 0; i--) {
    if (fullName[i] == Ion::Storage::k_dotChar) {
      dot = fullName + i;
      break;
    }
  }
  if (dot) {
    size_t baseLen = (size_t)(dot - fullName);
    if (baseLen >= sizeof(base)) baseLen = sizeof(base) - 1;
    for (size_t j = 0; j < baseLen; j++) {
      base[j] = fullName[j];
    }
    base[baseLen] = '\0';
    extension = dot + 1;
  } else {
    size_t copyLen = len;
    if (copyLen >= sizeof(base)) copyLen = sizeof(base) - 1;
    for (size_t j = 0; j < copyLen; j++) {
      base[j] = fullName[j];
    }
    base[copyLen] = '\0';
  }
  Ion::Storage::Record::ErrorStatus err = record.setName(fullName);
  if (err == Ion::Storage::Record::ErrorStatus::None) {
    StackViewController * stack = static_cast<StackViewController *>(parentResponder());
    ViewController * top = stack->topViewController();
    if (top) {
      top->viewWillAppear();
    }
  } else if (err == Ion::Storage::Record::ErrorStatus::NameTaken) {
    Container::activeApp()->displayWarning(I18n::Message::NameTaken);
  } else if (err == Ion::Storage::Record::ErrorStatus::NonCompliantName) {
    Container::activeApp()->displayWarning(I18n::Message::AllowedCharactersaz09, I18n::Message::NameCannotStartWithNumber);
  } else {
    Container::activeApp()->displayWarning(I18n::Message::NameTooLong);
  }
}


