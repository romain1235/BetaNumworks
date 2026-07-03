#ifndef SEQUENCE_TYPE_PARAMATER_CONTROLLER_H
#define SEQUENCE_TYPE_PARAMATER_CONTROLLER_H

#include <escher.h>
#include <poincare/layout.h>
#include "../../shared/sequence_store.h"

namespace Sequence {

class ListController;

class SequenceTypeTableCell : public ExpressionTableCellWithPointer {
public:
  using ExpressionTableCellWithPointer::ExpressionTableCellWithPointer;
protected:
  bool shouldInsetContentForRoundedCorners() const override { return true; }
  bool useUniformRoundedCornerContentInsets() const override { return true; }
};

class TypeParameterController : public ViewController, public SimpleListViewDataSource, public SelectableTableViewDataSource {
public:
  TypeParameterController(Responder * parentResponder, ListController * list,
    TableCell::Layout cellLayout, KDCoordinate topMargin = 0, KDCoordinate rightMargin = 0,
    KDCoordinate bottomMargin = 0, KDCoordinate leftMargin = 0);
  const char * title() override;
  View * view() override;
  void viewWillAppear() override;
  void viewDidDisappear() override;
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;
  int numberOfRows() const override;
  KDCoordinate cellHeight() override;
  HighlightCell * reusableCell(int index) override;
  int reusableCellCount() const override;
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;
  KDColor listBorderBackgroundColor() const override;
  bool listCellsHaveRoundedCorners() const override { return !m_record.isNull(); }
  void setRecord(Ion::Storage::Record record);
private:
  StackViewController * stackController() const;
  Shared::Sequence * sequence() {
    assert(!m_record.isNull());
    return sequenceStore()->modelForRecord(m_record);
  }
  Shared::SequenceStore * sequenceStore();
  constexpr static int k_totalNumberOfCell = 3;
  SequenceTypeTableCell m_explicitCell;
  SequenceTypeTableCell m_singleRecurrenceCell;
  SequenceTypeTableCell m_doubleRecurrenceCell;
  Poincare::Layout m_layouts[k_totalNumberOfCell];
  SelectableTableView m_selectableTableView;
  Ion::Storage::Record m_record;
  ListController * m_listController;
};

}

#endif
