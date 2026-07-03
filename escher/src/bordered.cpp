#include <escher/bordered.h>

uint8_t listSquareCornersForIndex(int index, int numberOfRows) {
  if (numberOfRows <= 1) {
    return 0;
  }
  if (index == 0) {
    return KDSquareCornerBottomLeft | KDSquareCornerBottomRight;
  }
  if (index == numberOfRows - 1) {
    return KDSquareCornerTopLeft | KDSquareCornerTopRight;
  }
  return KDSquareCornerAll;
}

uint8_t listSquareCornersForTableWithFooterRow(int index, int numberOfRows) {
  if (numberOfRows <= 1) {
    return 0;
  }
  if (index == numberOfRows - 1) {
    return 0;
  }
  if (index == 0) {
    if (numberOfRows == 2) {
      return 0;
    }
    return KDSquareCornerBottomLeft | KDSquareCornerBottomRight;
  }
  if (index == numberOfRows - 2) {
    return KDSquareCornerTopLeft | KDSquareCornerTopRight;
  }
  return KDSquareCornerAll;
}

uint8_t listSquareCornersForTableWithDetachedFooter(int index, int numberOfRows) {
  return listSquareCornersForIndex(index, numberOfRows);
}

void Bordered::drawBorderOfRect(KDContext * ctx, KDRect rect, KDColor borderColor, KDColor borderBackgroundColor, uint8_t squareCorners, KDColor innerColor) const {
  KDCoordinate width = rect.width();
  KDCoordinate height = rect.height();
  if (width == 0 || height == 0) {
    return;
  }
  if (squareCorners == KDSquareCornerAll) {
    ctx->fillRect(KDRect(0, 0, width, k_separatorThickness), borderColor);
    ctx->fillRect(KDRect(0, k_separatorThickness, k_separatorThickness, height - k_separatorThickness), borderColor);
    ctx->fillRect(KDRect(width - k_separatorThickness, k_separatorThickness, k_separatorThickness, height - k_separatorThickness), borderColor);
    ctx->fillRect(KDRect(0, height - k_separatorThickness, width, k_separatorThickness), borderColor);
    return;
  }
  KDRect outer(0, 0, width, height);
  ctx->fillRoundedRect(outer, k_cornerRadius, borderColor, borderBackgroundColor, squareCorners);
  if (width <= 2 * k_separatorThickness || height <= 2 * k_separatorThickness) {
    return;
  }
  KDRect inner(k_separatorThickness, k_separatorThickness, width - 2 * k_separatorThickness, height - 2 * k_separatorThickness);
  KDCoordinate innerRadius = k_cornerRadius > k_separatorThickness ? k_cornerRadius - k_separatorThickness : 0;
  ctx->fillRoundedRect(inner, innerRadius, innerColor, borderColor, squareCorners);
}

void Bordered::drawInnerRect(KDContext * ctx, KDRect rect, KDColor backgroundColor, uint8_t squareCorners) const {
  KDCoordinate width = rect.width();
  KDCoordinate height = rect.height();
  if (width <= 2 * k_separatorThickness || height <= 2 * k_separatorThickness) {
    return;
  }
  KDRect inner(k_separatorThickness, k_separatorThickness, width - 2 * k_separatorThickness, height - 2 * k_separatorThickness);
  if (squareCorners == KDSquareCornerAll) {
    ctx->fillRect(inner, backgroundColor);
    return;
  }
  KDCoordinate innerRadius = k_cornerRadius > k_separatorThickness ? k_cornerRadius - k_separatorThickness : 0;
  ctx->fillRoundedRect(inner, innerRadius, backgroundColor, backgroundColor, squareCorners);
}
