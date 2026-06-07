#ifndef FILES_MAIN_CONTROLLER_H
#define FILES_MAIN_CONTROLLER_H

#include <escher.h>
#include "file_name_cell.h"
#include "files_parameter_controller.h"

namespace Files {

class MainController : public ViewController, public ListViewDataSource, public SelectableTableViewDataSource, public TextFieldDelegate {
public:
  MainController(Responder * parentResponder, ::App * app);
  View * view() override;
  bool handleEvent(Ion::Events::Event event) override;
  void didBecomeFirstResponder() override;
  void renameSelectedFile();
  /* TextFieldDelegate */
  bool textFieldShouldFinishEditing(TextField * textField, Ion::Events::Event event) override;
  bool textFieldDidFinishEditing(TextField * textField, const char * text, Ion::Events::Event event) override;
  bool textFieldDidAbortEditing(TextField * textField) override;
  bool textFieldDidReceiveEvent(TextField * textField, Ion::Events::Event event) override;
  int numberOfRows() const override;
  KDCoordinate rowHeight(int j) override;
  KDCoordinate cumulatedHeightFromIndex(int j) override;
  int indexFromCumulatedHeight(KDCoordinate offsetY) override;
  HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override;
  int typeAtLocation(int i, int j) override;
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;
  void viewWillAppear() override;
private:
  ::App * m_app;
  SelectableTableView m_selectableTableView;
  int m_numberOfRows = 1;
  constexpr static int k_maxNumberOfCells = 8;
  FileNameCell m_cells[k_maxNumberOfCells];
  Files::FilesParameterController m_parameterController;
};

}

#endif
