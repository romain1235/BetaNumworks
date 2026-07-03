#ifndef ESCHER_BORDERED_H
#define ESCHER_BORDERED_H

#include <escher/metric.h>
#include <kandinsky/context.h>

constexpr uint8_t KDSquareCornerAll = KDSquareCornerTopLeft | KDSquareCornerTopRight | KDSquareCornerBottomLeft | KDSquareCornerBottomRight;

uint8_t listSquareCornersForIndex(int index, int numberOfRows);
uint8_t listSquareCornersForTableWithFooterRow(int index, int numberOfRows);
uint8_t listSquareCornersForTableWithDetachedFooter(int index, int numberOfRows);

class Bordered {
public:
  void drawBorderOfRect(KDContext * ctx, KDRect rect, KDColor borderColor, KDColor borderBackgroundColor, uint8_t squareCorners = KDSquareCornerAll, KDColor innerColor = KDColorWhite) const;
  void drawInnerRect(KDContext * ctx, KDRect rect, KDColor backgroundColor, uint8_t squareCorners = KDSquareCornerAll) const;
protected:
  constexpr static KDCoordinate k_cornerRadius = 4;
  constexpr static KDCoordinate k_separatorThickness = Metric::CellSeparatorThickness;
};

#endif
