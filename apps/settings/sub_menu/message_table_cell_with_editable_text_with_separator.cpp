#include "../message_table_cell_with_editable_text_with_separator.h"
#include "../../shared/button_with_separator.h"
#include <escher/metric.h>

namespace Settings {

MessageTableCellWithEditableTextWithSeparator::MessageTableCellWithEditableTextWithSeparator(Responder * parentResponder, InputEventHandlerDelegate * inputEventHandlerDelegate, TextFieldDelegate * textFieldDelegate, I18n::Message message) :
  CellWithSeparator(),
  m_cell(parentResponder, inputEventHandlerDelegate, textFieldDelegate, message),
  m_squareCorners(KDSquareCornerAll),
  m_borderBackgroundColor(Palette::ListCellBackground)
{
}

void MessageTableCellWithEditableTextWithSeparator::configureListAppearance(uint8_t squareCorners, KDColor borderBackgroundColor) {
  if (m_squareCorners != squareCorners || m_borderBackgroundColor != borderBackgroundColor) {
    m_squareCorners = squareCorners;
    m_borderBackgroundColor = borderBackgroundColor;
    m_cell.configureListAppearance(squareCorners, borderBackgroundColor);
    layoutSubviews(true);
    reloadCell();
  }
}

void MessageTableCellWithEditableTextWithSeparator::setHighlighted(bool highlight) {
  m_cell.setHighlighted(highlight);
  HighlightCell::setHighlighted(highlight);
}

void MessageTableCellWithEditableTextWithSeparator::drawRect(KDContext * ctx, KDRect rect) const {
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  if (m_squareCorners == KDSquareCornerAll) {
    ctx->fillRect(KDRect(0, 0, width, Metric::CellSeparatorThickness), Palette::ListCellBorder);
    ctx->fillRect(KDRect(0, Metric::CellSeparatorThickness, width, k_margin - Metric::CellSeparatorThickness), Palette::BackgroundApps);
    return;
  }
  ctx->fillRect(KDRect(0, 0, width, height), m_borderBackgroundColor);
}

int MessageTableCellWithEditableTextWithSeparator::numberOfSubviews() const {
  return 1;
}

View * MessageTableCellWithEditableTextWithSeparator::subviewAtIndex(int index) {
  assert(index == 0);
  return &m_cell;
}

void MessageTableCellWithEditableTextWithSeparator::layoutSubviews(bool force) {
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  constexpr KDCoordinate k_cornerRadius = 4;
  KDCoordinate topGap = m_squareCorners == 0 ? ButtonWithSeparator::k_topGap : k_margin;
  KDCoordinate horizontalInset = Metric::CellSeparatorThickness;
  if (m_squareCorners != KDSquareCornerAll) {
    horizontalInset += k_cornerRadius - Metric::CellSeparatorThickness;
  }
  m_cell.setFrame(KDRect(horizontalInset, topGap, width - 2 * horizontalInset, height - topGap), force);
}

}
