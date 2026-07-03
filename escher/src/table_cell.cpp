#include <escher/table_cell.h>
#include <escher/palette.h>
#include <escher/metric.h>
#include <algorithm>

TableCell::TableCell(Layout layout) :
  Bordered(),
  HighlightCell(),
  m_layout(layout),
  m_squareCorners(KDSquareCornerAll),
  m_borderBackgroundColor(Palette::ListCellBackground)
{
}

View * TableCell::labelView() const {
  return nullptr;
}

View * TableCell::accessoryView() const {
  return nullptr;
}

View * TableCell::subAccessoryView() const {
  return nullptr;
}

int TableCell::numberOfSubviews() const {
  return (labelView() != nullptr) + (accessoryView()!= nullptr) + (subAccessoryView()!= nullptr);
}

View * TableCell::subviewAtIndex(int index) {
  if (index == 0) {
    return labelView();
  }
  if (index == 1) {
    return accessoryView();
  }
  return subAccessoryView();
}

/*TODO: uniformize where margins are added. Sometimes the subview has included
 * margins (like ExpressionView), sometimes the subview has no margins (like
 * MessageView) which prevents us to handle margins only here. */

KDCoordinate withMargin(KDCoordinate length, KDCoordinate margin) {
  return length == 0 ? 0 : length + margin;
}

TableCell::Layout TableCell::resolvedLayout(KDCoordinate width, KDSize labelSize, KDSize subAccessorySize, KDSize accessorySize) const {
  if (m_layout != Layout::Adaptive) {
    return m_layout;
  }
  /* Width needed to lay everything out horizontally without any subview
   * overlapping another one (same spacing as the horizontal branch below). */
  KDCoordinate neededWidth = 2 * k_separatorThickness
    + withMargin(labelSize.width(), 2 * labelMargin())
    + withMargin(subAccessorySize.width(), k_horizontalMargin)
    + withMargin(accessorySize.width(), accessoryMargin());
  return neededWidth <= width ? Layout::HorizontalRightOverlap : Layout::Vertical;
}

KDCoordinate TableCell::minimalHeightForOptimalDisplay(KDCoordinate width, KDSize labelSize, KDSize subAccessorySize, KDSize accessorySize) const {
  Layout layout = resolvedLayout(width, labelSize, subAccessorySize, accessorySize);
  if (layout == Layout::Vertical) {
    // Label on top, then the accessories stacked below it.
    return 2 * k_separatorThickness + k_verticalMargin
      + labelSize.height() + k_verticalMargin
      + subAccessorySize.height()
      + accessorySize.height() + k_verticalMargin;
  }
  // Everything on a single line, vertically centered.
  KDCoordinate maxHeight = std::max(labelSize.height(), std::max(subAccessorySize.height(), accessorySize.height()));
  return 2 * k_separatorThickness + 2 * k_verticalMargin + maxHeight;
}

TableCell::Layout TableCell::effectiveLayout() const {
  if (m_layout != Layout::Adaptive) {
    return m_layout;
  }
  View * label = labelView();
  View * accessory = accessoryView();
  View * subAccessory = subAccessoryView();
  KDSize labelSize = label ? label->minimalSizeForOptimalDisplay() : KDSizeZero;
  KDSize accessorySize = accessory ? accessory->minimalSizeForOptimalDisplay() : KDSizeZero;
  KDSize subAccessorySize = subAccessory ? subAccessory->minimalSizeForOptimalDisplay() : KDSizeZero;
  return resolvedLayout(bounds().width(), labelSize, subAccessorySize, accessorySize);
}

void TableCell::roundedCornerContentInsets(KDCoordinate & extraLeft, KDCoordinate & extraTop, KDCoordinate & extraRight, KDCoordinate & extraBottom) const {
  extraLeft = 0;
  extraTop = 0;
  extraRight = 0;
  extraBottom = 0;
}

void TableCell::layoutSubviews(bool force) {
  /* TODO: this code is awful. However, this should handle multiples cases
   * (subviews are not defined, margins are overriden...) */
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  View * label = labelView();
  View * accessory = accessoryView();
  View * subAccessory = subAccessoryView();
  KDSize labelSize = label ? label->minimalSizeForOptimalDisplay() : KDSizeZero;
  KDSize accessorySize = accessory ? accessory->minimalSizeForOptimalDisplay() : KDSizeZero;
  KDSize subAccessorySize = subAccessory ? subAccessory->minimalSizeForOptimalDisplay() : KDSizeZero;
  Layout layout = effectiveLayout();
  KDCoordinate extraLeft = 0;
  KDCoordinate extraTop = 0;
  KDCoordinate extraRight = 0;
  KDCoordinate extraBottom = 0;
  roundedCornerContentInsets(extraLeft, extraTop, extraRight, extraBottom);
  KDCoordinate innerTop = k_separatorThickness + extraTop;
  KDCoordinate innerBottom = k_separatorThickness + extraBottom;
  KDCoordinate innerLeft = k_separatorThickness + extraLeft;
  KDCoordinate innerRight = k_separatorThickness + extraRight;
  KDCoordinate innerHeight = height - innerTop - innerBottom;
  if (layout == Layout::Vertical) {
    /*
     * Vertically:
     * ----------------
     * ----------------
     * Line separator
     * ----------------
     * k_verticalMargin
     * ----------------
     *     LABEL
     * ----------------
     * k_verticalMargin
     * ----------------
     *       .
     *       . [White space if possible, otherwise LABEL overlaps SUBACCESSORY and so on]
     *       .
     * ----------------
     *  SUBACCESSORY
     * ----------------
     *   ACCESSORY
     * ----------------
     * k_verticalMargin
     * ----------------
     * Line separator
     * ----------------
     * ----------------
     *
     *
     *  Horizontally:
     * || Line separator | margin* | SUBVIEW | margin* | Line separator ||
     *
     * * = margin can either be labelMargin(), accessoryMargin() or k_horizontalMargin depending on the subview
     *
     * */
    KDCoordinate horizontalMargin = innerLeft + labelMargin();
    KDCoordinate y = innerTop;
    if (label) {
      y += k_verticalMargin;
      KDCoordinate labelHeight = std::min<KDCoordinate>(labelSize.height(), height - y - innerBottom - k_verticalMargin);
      label->setFrame(KDRect(horizontalMargin, y, width - horizontalMargin - innerRight - labelMargin(), labelHeight), force);
      y += labelHeight + k_verticalMargin;
    }
    horizontalMargin = innerLeft + k_horizontalMargin;
    y = std::max<KDCoordinate>(y, height - innerBottom - withMargin(accessorySize.height(), k_verticalMargin) - withMargin(subAccessorySize.height(), 0));
    if (subAccessory) {
      KDCoordinate subAccessoryHeight = std::min<KDCoordinate>(subAccessorySize.height(), height - y - innerBottom - k_verticalMargin);
      subAccessory->setFrame(KDRect(horizontalMargin, y, width - horizontalMargin - innerRight, subAccessoryHeight), force);
      y += subAccessoryHeight;
    }
    horizontalMargin = innerLeft + accessoryMargin();
    y = std::max<KDCoordinate>(y, height - innerBottom - withMargin(accessorySize.height(), k_verticalMargin));
    if (accessory) {
      KDCoordinate accessoryHeight = std::min<KDCoordinate>(accessorySize.height(), height - y - innerBottom - k_verticalMargin);
      accessory->setFrame(KDRect(horizontalMargin, y, width - horizontalMargin - innerRight, accessoryHeight), force);
    }
  } else {
    /*
     * Vertically:
     * ----------------
     * ----------------
     * Line separator
     * ----------------
     *    SUBVIEW
     * ----------------
     * Line separator
     * ----------------
     * ----------------
     *
     *  Horizontally:
     * || Line separator | Label margin | LABEL | Label margin | ...
     *      [ White space if possible otherwise the overlap can be from left to
     *      right subviews or the contrary ]
     *
     *  ... | SUBACCESSORY | ACCESSORY | Accessory margin | Line separator ||
     *
     * */

    KDCoordinate x = 0;
    KDCoordinate labelX = innerLeft + labelMargin();
    KDCoordinate subAccessoryX = std::max(innerLeft + k_horizontalMargin, width - innerRight - withMargin(accessorySize.width(), accessoryMargin()) - withMargin(subAccessorySize.width(), 0));
    KDCoordinate accessoryX = std::max(innerLeft + accessoryMargin(), width - innerRight - withMargin(accessorySize.width(), accessoryMargin()));
    if (label) {
      x = labelX;
      KDCoordinate labelWidth = std::min<KDCoordinate>(labelSize.width(), width - x - innerRight - labelMargin());
      if (layout == Layout::HorizontalRightOverlap) {
        labelWidth = std::min<KDCoordinate>(labelWidth, subAccessoryX - x - labelMargin());
      }
      label->setFrame(KDRect(x, innerTop, labelWidth, innerHeight), force);
      x += labelWidth + labelMargin();
    }
    if (subAccessory) {
      x = std::max(x, subAccessoryX);
      KDCoordinate subAccessoryWidth = std::min<KDCoordinate>(subAccessorySize.width(), width - x - innerRight - k_horizontalMargin);
      if (layout == Layout::HorizontalRightOverlap) {
        subAccessoryWidth = std::min<KDCoordinate>(subAccessoryWidth, accessoryX - x);
      }
      subAccessory->setFrame(KDRect(x, innerTop, subAccessoryWidth, innerHeight), force);
      x += subAccessoryWidth;
    }
    if (accessory) {
      x = std::max(x, accessoryX);
      KDCoordinate accessoryWidth = std::min<KDCoordinate>(accessorySize.width(), width - x - innerRight - accessoryMargin());
      accessory->setFrame(KDRect(x, innerTop, accessoryWidth, innerHeight), force);
    }
  }
}

void TableCell::configureListAppearance(uint8_t squareCorners, KDColor borderBackgroundColor) {
  if (m_squareCorners != squareCorners || m_borderBackgroundColor != borderBackgroundColor) {
    m_squareCorners = squareCorners;
    m_borderBackgroundColor = borderBackgroundColor;
    layoutSubviews();
  }
}

void TableCell::drawRect(KDContext * ctx, KDRect rect) const {
  KDColor backColor = isHighlighted() ? Palette::ListCellBackgroundSelected : Palette::ListCellBackground;
  KDRect cellBounds = bounds();
  if (m_squareCorners == KDSquareCornerAll) {
    drawInnerRect(ctx, cellBounds, backColor);
    drawBorderOfRect(ctx, cellBounds, Palette::ListCellBorder, m_borderBackgroundColor);
    return;
  }
  ctx->fillRect(cellBounds, m_borderBackgroundColor);
  drawBorderOfRect(ctx, cellBounds, Palette::ListCellBorder, m_borderBackgroundColor, m_squareCorners, backColor);
}
