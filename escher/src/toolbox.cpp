#include <escher/toolbox.h>
#include <escher/metric.h>
#include <kandinsky/font.h>
#include <assert.h>
#include <string.h>

Toolbox::Toolbox(Responder * parentResponder, I18n::Message title) :
  NestedMenuController(parentResponder, title),
  m_messageTreeModel(nullptr),
  m_savedStack(),
  m_savedSelectedRow(0),
  m_savedVerticalScroll(0),
  m_hasSavedState(false)
{}

void Toolbox::viewWillAppear() {
  m_messageTreeModel = (ToolboxMessageTree *)rootModel();

  // Re-apply current palette colors when the toolbox is shown.
  // Reusable cells can outlive theme changes, so their text colors must be
  // refreshed from the active palette.
  for (int i = 0; i < maxNumberOfDisplayedRows(); i++) {
    refreshLeafCellAppearance(i);
    auto * nodeCell = nodeCellAtIndex(i);
    nodeCell->setTextColor(Palette::PrimaryText);
    // Re-run highlight painting to refresh message/accessory backgrounds from
    // current palette values (selected and non-selected states).
    nodeCell->setHighlighted(nodeCell->isHighlighted());
  }

  NestedMenuController::viewWillAppear();

  if (!m_hasSavedState) {
    return;
  }

  m_stack = m_savedStack;
  m_messageTreeModel = static_cast<const ToolboxMessageTree *>(rootModel());
  for (int i = 0; i < m_stack.depth(); i++) {
    NestedMenuController::Stack::State * state = m_stack.stateAtIndex(i);
    m_messageTreeModel = static_cast<const ToolboxMessageTree *>(m_messageTreeModel->childAtIndex(state->selectedRow()));
    if (m_messageTreeModel->isFork()) {
      m_messageTreeModel = static_cast<const ToolboxMessageTree *>(m_messageTreeModel->childAtIndex(indexAfterFork()));
    }
  }

  m_selectableTableView.reloadData();
  int firstSelectedRow = m_savedSelectedRow;
  if (firstSelectedRow < 0) {
    firstSelectedRow = 0;
  }
  int lastRow = numberOfRows() - 1;
  if (lastRow >= 0 && firstSelectedRow > lastRow) {
    firstSelectedRow = lastRow;
  }
  m_listController.setFirstSelectedRow(firstSelectedRow);

  KDPoint offset = m_selectableTableView.contentOffset();
  m_selectableTableView.setContentOffset(KDPoint(offset.x(), m_savedVerticalScroll));
}

void Toolbox::viewDidDisappear() {
  m_savedStack = m_stack;
  m_savedSelectedRow = selectedRow() < 0 ? 0 : selectedRow();
  m_savedVerticalScroll = m_selectableTableView.contentOffset().y();
  m_hasSavedState = true;
  NestedMenuController::viewDidDisappear();
}

void Toolbox::refreshLeafCellAppearance(int i) {
  auto * leafCell = static_cast<MessageTableCellWithMessage<SlideableMessageTextView> *>(leafCellAtIndex(i));
  leafCell->setTextColor(Palette::PrimaryText);
  leafCell->setAccessoryTextColor(Palette::SecondaryText);
  leafCell->setHighlighted(leafCell->isHighlighted());
}

int Toolbox::numberOfRows() const {
  if (m_messageTreeModel == nullptr) {
    m_messageTreeModel = (ToolboxMessageTree *)rootModel();
  }
  return m_messageTreeModel->numberOfChildren();
}

KDCoordinate Toolbox::rowHeight(int j) {
  if (m_messageTreeModel == nullptr) {
    m_messageTreeModel = (ToolboxMessageTree *)rootModel();
  }
  const ToolboxMessageTree * messageTree = static_cast<const ToolboxMessageTree *>(m_messageTreeModel->childAtIndex(j));
  /* Height needed to display a single line of text (the label, possibly with a
   * description shown side by side with it). */
  KDCoordinate singleLineHeight = KDFont::LargeFont->glyphSize().height() + 2 * Metric::TableCellVerticalMargin + 2 * Metric::CellSeparatorThickness;
  bool isLeaf = messageTree->numberOfChildren() == 0;
  bool hasDescription = isLeaf && messageTree->text() != static_cast<I18n::Message>(0);
  if (!hasDescription) {
    return singleLineHeight;
  }
  /* The label and its description are shown side by side when there is enough
   * room, and stacked vertically otherwise. The "fits side by side" test below
   * must stay in sync with TableCell::effectiveLayout. */
  KDCoordinate width = m_selectableTableView.bounds().width() - m_selectableTableView.leftMargin() - m_selectableTableView.rightMargin();
  KDSize labelSize = KDFont::LargeFont->stringSize(I18n::translate(messageTree->label()));
  KDSize descriptionSize = KDFont::SmallFont->stringSize(I18n::translate(messageTree->text()));
  KDCoordinate neededWidth = 2 * Metric::CellSeparatorThickness
    + labelSize.width() + 2 * Metric::TableCellHorizontalMargin
    + descriptionSize.width() + Metric::TableCellHorizontalMargin;
  if (width > 0 && neededWidth > width) {
    // Not enough room: label and description are stacked vertically.
    return Metric::ToolboxRowHeight;
  }
  return singleLineHeight;
}

int Toolbox::reusableCellCount(int type) {
  return maxNumberOfDisplayedRows();
}

void Toolbox::willDisplayCellForIndex(HighlightCell * cell, int index) {
  ToolboxMessageTree * messageTree = (ToolboxMessageTree *)m_messageTreeModel->childAtIndex(index);
  if (messageTree->numberOfChildren() == 0) {
    MessageTableCellWithMessage<SlideableMessageTextView> * myCell = (MessageTableCellWithMessage<SlideableMessageTextView> *)cell;
    myCell->setMessage(messageTree->label());
    myCell->setAccessoryMessage(messageTree->text());
    myCell->setAccessoryTextColor(Palette::SecondaryText);
    return;
  } else {
    MessageTableCell<> * myCell = (MessageTableCell<> *)cell;
    myCell->setMessage(messageTree->label());
    myCell->reloadCell();
  }
}

int Toolbox::typeAtLocation(int i, int j) {
  const MessageTree * messageTree = m_messageTreeModel->childAtIndex(j);
  if (messageTree->numberOfChildren() == 0) {
    return LeafCellType;
  }
  return NodeCellType;
}

bool Toolbox::selectSubMenu(int selectedRow) {
  m_selectableTableView.deselectTable();
  m_messageTreeModel = static_cast<const ToolboxMessageTree *>(m_messageTreeModel->childAtIndex(selectedRow));
  if (m_messageTreeModel->isFork()) {
    assert(m_messageTreeModel->numberOfChildren() < 0);
    m_messageTreeModel = static_cast<const ToolboxMessageTree *>(m_messageTreeModel->childAtIndex(indexAfterFork()));
  }
  return NestedMenuController::selectSubMenu(selectedRow);
}

bool Toolbox::returnToPreviousMenu() {
  m_selectableTableView.deselectTable();
  int currentDepth = m_stack.depth();
  int index = 0;
  ToolboxMessageTree * parentMessageTree = (ToolboxMessageTree *)rootModel();
  Stack::State * previousState = m_stack.stateAtIndex(index++);
  while (currentDepth-- > 1) {
    parentMessageTree = (ToolboxMessageTree *)parentMessageTree->childAtIndex(previousState->selectedRow());
    previousState = m_stack.stateAtIndex(index++);
    if (parentMessageTree->isFork()) {
      parentMessageTree = (ToolboxMessageTree *)parentMessageTree->childAtIndex(indexAfterFork());
    }
  }
  m_messageTreeModel = parentMessageTree;
  return NestedMenuController::returnToPreviousMenu();
}
