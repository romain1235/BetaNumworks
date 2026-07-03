#ifndef ESCHER_TABLE_CELL_H
#define ESCHER_TABLE_CELL_H

#include <escher/bordered.h>
#include <escher/highlight_cell.h>

class TableCell : public Bordered, public HighlightCell {
public:
  /* Layout enum class determines the way subviews are layouted.
   * We can split the cell vertically or horizontally.
   * We can choose which subviews frames are optimized (if there is not enough
   * space for all subviews, which one is cropped). This case happens so far only
   * for horizontally splitted cell, so we distinguish only these sub cases.
   * TODO: implement VerticalTopOverlap, VerticalBottomlap? */
  enum class Layout {
    Vertical,
    HorizontalLeftOverlap, // Label overlaps on SubAccessory which overlaps on Accessory
    HorizontalRightOverlap, // Reverse
    Adaptive, // Horizontal if every subview fits side by side, Vertical otherwise
  };
  TableCell(Layout layout = Layout::HorizontalLeftOverlap);
  void setLayoutType(Layout layout) { m_layout = layout; }
  virtual View * labelView() const;
  virtual View * accessoryView() const;
  virtual View * subAccessoryView() const;
  void drawRect(KDContext * ctx, KDRect rect) const override;
  void configureListAppearance(uint8_t squareCorners, KDColor borderBackgroundColor) override;
  /* Resolves the layout actually used: for Adaptive, picks Horizontal when all
   * subviews fit side by side, Vertical otherwise. */
  Layout effectiveLayout() const;
  /* Same resolution but from explicit subview sizes and an available width.
   * Lets a data source predict the layout/height without configuring the cell. */
  Layout resolvedLayout(KDCoordinate width, KDSize labelSize, KDSize subAccessorySize, KDSize accessorySize) const;
  KDCoordinate minimalHeightForOptimalDisplay(KDCoordinate width, KDSize labelSize, KDSize subAccessorySize, KDSize accessorySize) const;
protected:
  virtual KDColor backgroundColor() const { return KDColorWhite; }
  virtual KDCoordinate labelMargin() const { return k_horizontalMargin; }
  virtual KDCoordinate accessoryMargin() const { return k_horizontalMargin; }
  int numberOfSubviews() const override;
  View * subviewAtIndex(int index) override;
  void layoutSubviews(bool force = false) override;
  virtual bool shouldInsetContentForRoundedCorners() const { return false; }
  virtual bool useUniformRoundedCornerContentInsets() const { return false; }
  constexpr static KDCoordinate k_verticalMargin = Metric::TableCellVerticalMargin;
  constexpr static KDCoordinate k_horizontalMargin = Metric::TableCellHorizontalMargin;
private:
  void roundedCornerContentInsets(KDCoordinate & extraLeft, KDCoordinate & extraTop, KDCoordinate & extraRight, KDCoordinate & extraBottom) const;
  Layout m_layout;
  uint8_t m_squareCorners;
  KDColor m_borderBackgroundColor;
};

#endif

