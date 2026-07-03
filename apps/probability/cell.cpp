#include "cell.h"
#include <assert.h>

namespace Probability {

Cell::Cell() :
  Bordered(),
  HighlightCell(),
  m_labelView(KDFont::LargeFont, (I18n::Message)0, 0, 0.5, Palette::PrimaryText, Palette::BackgroundHard),
  m_icon(nullptr),
  m_focusedIcon(nullptr),
  m_squareCorners(KDSquareCornerAll),
  m_borderBackgroundColor(Palette::BackgroundApps)
{
}

int Cell::numberOfSubviews() const {
  return 3;
}

View * Cell::subviewAtIndex(int index) {
  assert(index >= 0 && index < 3);
  if (index == 0) {
    return &m_labelView;
  }
  if (index == 1) {
    return &m_iconView;
  }
  return &m_chevronView;
}

void Cell::layoutSubviews(bool force) {
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  m_labelView.setFrame(KDRect(1+k_iconWidth+2*k_iconMargin, 1, width-2-k_iconWidth-2*k_iconMargin - k_chevronWidth, height-2), force);
  m_iconView.setFrame(KDRect(1+k_iconMargin, (height - k_iconHeight)/2, k_iconWidth, k_iconHeight), force);
  m_chevronView.setFrame(KDRect(width-1-k_chevronWidth-k_chevronMargin, 1, k_chevronWidth, height - 2), force);
}

void Cell::reloadCell() {
  HighlightCell::reloadCell();
  KDColor backgroundColor = isHighlighted()? Palette::ListCellBackgroundSelected : Palette::ListCellBackground;
  m_labelView.setBackgroundColor(backgroundColor);
  if (isHighlighted()) {
    m_iconView.setImage(m_focusedIcon);
  } else {
    m_iconView.setImage(m_icon);
  }
}

void Cell::setLabel(I18n::Message message) {
  m_labelView.setMessage(message);
}

void Cell::setImage(const Image * image, const Image * focusedImage) {
  m_icon = image;
  m_focusedIcon = focusedImage;
}

void Cell::configureListAppearance(uint8_t squareCorners, KDColor borderBackgroundColor) {
  m_squareCorners = squareCorners;
  m_borderBackgroundColor = borderBackgroundColor;
}

void Cell::drawRect(KDContext * ctx, KDRect rect) const {
  KDColor backgroundColor = isHighlighted() ? Palette::ListCellBackgroundSelected : Palette::ListCellBackground;
  KDRect cellBounds = bounds();
  if (m_squareCorners == KDSquareCornerAll) {
    drawInnerRect(ctx, cellBounds, backgroundColor);
    drawBorderOfRect(ctx, cellBounds, Palette::ProbabilityCellBorder, m_borderBackgroundColor);
    return;
  }
  ctx->fillRect(cellBounds, m_borderBackgroundColor);
  drawBorderOfRect(ctx, cellBounds, Palette::ProbabilityCellBorder, m_borderBackgroundColor, m_squareCorners, backgroundColor);
}

}
