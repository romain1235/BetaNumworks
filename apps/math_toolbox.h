#ifndef APPS_MATH_TOOLBOX_H
#define APPS_MATH_TOOLBOX_H

#include <escher.h>
#include <apps/i18n.h>

class MathToolbox : public Toolbox {
public:
  MathToolbox();
  const ToolboxMessageTree * rootModel() const override;
protected:
  bool selectLeaf(int selectedRow, bool quitToolbox) override;
  MessageTableCellWithMessage<SlideableMessageTextView> * leafCellAtIndex(int index) override;
  MessageTableCellWithChevron<SlideableMessageTextView> * nodeCellAtIndex(int index) override;
  int maxNumberOfDisplayedRows() override;
  /* Rows can now be as short as a single line of text (label and description
   * shown side by side), so more of them fit on screen at once. = 240/26 + 1 */
  constexpr static int k_maxNumberOfDisplayedRows = 11;
private:
  int indexAfterFork() const override;

  MessageTableCellWithMessage<SlideableMessageTextView> m_leafCells[k_maxNumberOfDisplayedRows];
  MessageTableCellWithChevron<SlideableMessageTextView> m_nodeCells[k_maxNumberOfDisplayedRows];
};

#endif
