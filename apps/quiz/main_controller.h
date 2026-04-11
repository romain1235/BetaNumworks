#ifndef QUIZ_MAIN_CONTROLLER_H
#define QUIZ_MAIN_CONTROLLER_H

#include <escher.h>
#include <apps/i18n.h>

/*
 * MainController: a very small demonstrative menu for the Quiz app.
 * - Implements a simple one-column list with a few items.
 * - Pressing OK displays a short warning/pop-up with the selected item's
 *   message (via `displayWarning`) so you can see interaction without
 *   implementing full app logic.
 */
namespace Quiz {

class MainController : public StackViewController, public ListViewDataSource, public SelectableTableViewDataSource, public SelectableTableViewDelegate {
public:
  MainController();
  // Responder: handle user events (OK/EXE to select a row)
  bool handleEvent(Ion::Events::Event event) override;
  void didBecomeFirstResponder() override;

  // ListViewDataSource: sizing and display hooks for the list
  int numberOfRows() const override;
  int numberOfColumns() const override { return 1; }
  KDCoordinate columnWidth(int i) override { return cellWidth(); }
  void willDisplayCellAtLocation(HighlightCell * cell, int i, int j) override { willDisplayCellForIndex(cell, j); }
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;
  KDCoordinate rowHeight(int j) override;

  // TableView data-source helpers
  HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override;
  // single-column list only uses leaf cell type (0)
  int typeAtLocation(int i, int j) override { return 0; }

private:
  /* InnerListController is the ViewController that wraps the SelectableTableView
   * and is used as the root view of the StackViewController. Keeping the table
   * inside an inner controller is the standard pattern used across the codebase.
   */
  class InnerListController : public ViewController {
  public:
    InnerListController(MainController * dataSource);
    //const char * title() override { return I18n::translate(I18n::Message::About); }
    View * view() override { return &m_selectableTableView; }
    void didBecomeFirstResponder() override;
    // Expose the table so the outer controller can query selection
    SelectableTableView * selectableTableView() { return &m_selectableTableView; }
  private:
    SelectableTableView m_selectableTableView;
  };

  // Small fixed-size menu for demonstration
  constexpr static int k_maxNumberOfDisplayedRows = 6;
  constexpr static int k_numberOfMenuRows = 4;
  InnerListController m_listController; // the list view
  MessageTableCellWithChevron<> m_cells[k_maxNumberOfDisplayedRows]; // reusable cells
  // Messages shown for each menu row (translated via I18n)
  I18n::Message m_menuMessages[k_numberOfMenuRows];
};

}

#endif
