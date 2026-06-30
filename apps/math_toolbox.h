#ifndef APPS_MATH_TOOLBOX_H
#define APPS_MATH_TOOLBOX_H

#include <escher/toolbox.h>
#include <escher/expression_table_cell_with_pointer.h>
#include <escher/message_table_cell_with_message.h>
#include <escher/message_table_cell_with_chevron.h>
#include <apps/i18n.h>
#include <poincare/layout.h>

class MathToolbox : public Toolbox {
public:
  MathToolbox();
  const ToolboxMessageTree * rootModel() const override;
  void viewDidDisappear() override;
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;
  KDCoordinate rowHeight(int j) override;
  /* Row heights are not uniform (math layouts and wrapped descriptions vary), so
   * the table view would otherwise recompute every row height on each scroll to
   * accumulate offsets. We cache the prefix sums once per submenu to keep
   * scrolling - especially near the bottom of long menus - fast. */
  KDCoordinate cumulatedHeightFromIndex(int j) override;
  int indexFromCumulatedHeight(KDCoordinate offsetY) override;
  bool handleEvent(Ion::Events::Event event) override;
  HighlightCell * reusableCell(int index, int type) override;
  int typeAtLocation(int i, int j) override;
protected:
  bool handleToolboxRowEvent(Ion::Events::Event event, int rowIndex);
  bool selectLeaf(int selectedRow, bool quitToolbox) override;
  ExpressionTableCellWithPointer * leafCellAtIndex(int index) override;
  MessageTableCellWithChevron<SlideableMessageTextView> * nodeCellAtIndex(int index) override;
  void refreshLeafCellAppearance(int i) override;
  int maxNumberOfDisplayedRows() override;
  /* Rows can now be as short as a single line of text (label and description
   * shown side by side), so more of them fit on screen at once. = 240/26 + 1 */
  constexpr static int k_maxNumberOfDisplayedRows = 11;
  constexpr static int TextLeafCellType = 3;
  /* Upper bound on the number of rows of a single submenu (the periodic table
   * menus have 120 entries). Used to size the row-height cache. */
  constexpr static int k_maxNumberOfRows = 130;
private:
  int indexAfterFork() const override;
  static bool labelNeedsMathLayout(const char * text);
  KDCoordinate textLeafRowHeight(const ToolboxMessageTree * messageTree);
  void rebuildHeightCacheIfNeeded();
  /* Math leaf labels are rendered as 2D layouts. Layouts are memoized over a
   * sliding window of displayed rows, like MathVariableBoxController does. */
  Poincare::Layout layoutAtIndex(int index);
  Poincare::Layout createLayoutForIndex(int index);
  void resetMemoization();
  KDCoordinate cellContentWidth();

  ExpressionTableCellWithPointer m_leafCells[k_maxNumberOfDisplayedRows];
  MessageTableCellWithMessage<SlideableMessageTextView> m_textLeafCells[k_maxNumberOfDisplayedRows];
  MessageTableCellWithChevron<SlideableMessageTextView> m_nodeCells[k_maxNumberOfDisplayedRows];
  Poincare::Layout m_layouts[k_maxNumberOfDisplayedRows];
  int m_firstMemoizedLayoutIndex;
  /* Model the memoized layouts were built for; when it changes (navigation,
   * state restoration) the memoization is reset. */
  const ToolboxMessageTree * m_memoizedModel;
  /* Prefix sums of row heights for the current submenu: m_cumulatedHeights[k] is
   * the total height of rows [0, k). Rebuilt when the submenu changes. */
  KDCoordinate m_cumulatedHeights[k_maxNumberOfRows + 1];
  const ToolboxMessageTree * m_heightCacheModel;
  int m_heightCacheRowCount;
  bool m_heightCacheValid;
};

#endif
