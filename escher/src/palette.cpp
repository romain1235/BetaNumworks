#include <escher/palette.h>
#include <assert.h>

KDColor Palette::nextDataColor(int * colorIndex) {
  size_t nbOfColors = numberOfDataColors();
  assert((size_t)*colorIndex < nbOfColors);
  KDColor c = DataColor[*colorIndex];
  *colorIndex = (*colorIndex + 1) % nbOfColors;
  return c;
}
