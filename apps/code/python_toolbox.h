#ifndef CODE_PYTHON_TOOLBOX_H
#define CODE_PYTHON_TOOLBOX_H

#include <apps/i18n.h>
#include <escher.h>
#include <ion/events.h>
#include <kandinsky/font.h>
#include "toolbox_ion_keys.h"

namespace Code {

class PythonToolbox : public Toolbox {
public:
  // PythonToolbox
  PythonToolbox();
  const ToolboxMessageTree * moduleChildren(const char * name, int * numberOfNodes) const;

  // Toolbox
  bool handleEvent(Ion::Events::Event event) override;
  const ToolboxMessageTree * rootModel() const override;

  // ListViewDataSource
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;
  /* Row heights are not uniform (multi-line leaves vary), so the table view
   * would otherwise recompute every row height on each scroll to accumulate
   * offsets. We cache the prefix sums once per submenu to keep scrolling —
   * especially near the bottom of long menus like the catalog — fast. */
  KDCoordinate cumulatedHeightFromIndex(int j) override;
  int indexFromCumulatedHeight(KDCoordinate offsetY) override;

protected:
  KDCoordinate rowHeight(int j) override;
  bool selectLeaf(int selectedRow, bool quitToolbox) override;
  bool selectSubMenu(int selectedRow) override;
  MessageTableCellWithMessage<SlideableMessageTextView> * leafCellAtIndex(int index) override;
  MessageTableCellWithChevron<SlideableMessageTextView> * nodeCellAtIndex(int index) override;
  int maxNumberOfDisplayedRows() override;
  bool canStayInMenu() override { return true; }
  constexpr static int k_maxNumberOfDisplayedRows = 13; // = 240/(13+2*3)
  // 13 = minimal string height size
  // 3 = vertical margins
  /* Upper bound on the number of rows of a single submenu (the catalog has
   * 175 entries). Used to size the row-height cache. */
  constexpr static int k_maxNumberOfRows = 180;
private:
  constexpr static const KDFont * k_fontForMultiLine = KDFont::SmallFont;
  void scrollToLetter(char letter);
  void scrollToAndSelectChild(int i);
  void rebuildHeightCacheIfNeeded();
  MessageTableCellWithMessage<SlideableMessageTextView> m_leafCells[k_maxNumberOfDisplayedRows];
  MessageTableCellWithChevron<SlideableMessageTextView> m_nodeCells[k_maxNumberOfDisplayedRows];
  ToolboxIonKeys m_ionKeys;
  /* Prefix sums of row heights for the current submenu: m_cumulatedHeights[k] is
   * the total height of rows [0, k). Rebuilt when the submenu changes. */
  KDCoordinate m_cumulatedHeights[k_maxNumberOfRows + 1];
  const ToolboxMessageTree * m_heightCacheModel;
  int m_heightCacheRowCount;
  bool m_heightCacheValid;
};

}

#endif
