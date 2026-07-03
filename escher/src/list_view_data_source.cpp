#include <escher/list_view_data_source.h>
#include <escher/palette.h>
#include <escher/bordered.h>

KDCoordinate ListViewDataSource::cellWidth() {
  return 0;
}

KDCoordinate ListViewDataSource::columnWidth(int i) {
  return cellWidth();
}

int ListViewDataSource::numberOfColumns() const {
  return 1;
}

void ListViewDataSource::willDisplayCellAtLocation(HighlightCell * cell, int x, int y) {
  cell->configureListAppearance(listSquareCorners(y), listBorderBackgroundColor());
  willDisplayCellForIndex(cell, y);
}

void ListViewDataSource::willDisplayCellForIndex(HighlightCell * cell, int index) {
}

KDColor ListViewDataSource::listBorderBackgroundColor() const {
  return Palette::ListCellBackground;
}

bool ListViewDataSource::listCellsHaveRoundedCorners() const {
  return true;
}

uint8_t ListViewDataSource::listSquareCorners(int index) const {
  if (!listCellsHaveRoundedCorners()) {
    return KDSquareCornerAll;
  }
  return listSquareCornersForIndex(index, numberOfRows());
}

KDCoordinate ListViewDataSource::cumulatedWidthFromIndex(int i) {
  if (i == 1) {
    return cellWidth();
  }
  return 0;
}

int ListViewDataSource::indexFromCumulatedWidth(KDCoordinate offsetX) {
  return 0;
}
