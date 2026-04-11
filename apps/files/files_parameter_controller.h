#ifndef FILES_PARAMETER_CONTROLLER_H
#define FILES_PARAMETER_CONTROLLER_H

#include <escher.h>
#include <ion/storage.h>
#include "delete_pop_up_controller.h"

namespace Files {

class FilesParameterController : public ViewController, public SimpleListViewDataSource, public SelectableTableViewDataSource {
public:
  FilesParameterController(Responder * parentResponder);
  void setRecord(Ion::Storage::Record record) { m_record = record; }
  const char * title();
  bool handleEvent(Ion::Events::Event event) override;
  void viewWillAppear() override;
  void didBecomeFirstResponder() override;
  View * view() override { return &m_selectableTableView; }

  void renameRecord(Ion::Storage::Record record, const char * fullName);

  /* SimpleListViewDataSource */
  KDCoordinate cellHeight() override { return Metric::ParameterCellHeight; }
  HighlightCell * reusableCell(int index) override;
  int reusableCellCount() const override { return k_totalNumberOfCell; }
  int numberOfRows() const override { return k_totalNumberOfCell; }
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;

private:
  Ion::Storage::Record m_record;
  SelectableTableView m_selectableTableView;
  MessageTableCell<MessageTextView> m_deleteCell;
  MessageTableCell<MessageTextView> m_renameCell;
  constexpr static int k_totalNumberOfCell = 2;
  DeletePopUpController m_confirmPopUpController;
  
};

}

#endif
