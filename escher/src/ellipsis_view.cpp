#include <escher/ellipsis_view.h>
#include <escher/palette.h>
#include <kandinsky/color.h>

const uint8_t ellipsisMask[EllipsisView::k_ellipsisHeight][EllipsisView::k_ellipsisWidth] = {
  {0xFF, 0x7F, 0x5C, 0x74, 0xFF, 0xFF, 0xFF, 0x7F, 0x5C, 0x74, 0xFF, 0xFF, 0xFF, 0x7F, 0x5C, 0x74, 0xFF},
  {0xFF, 0x72, 0x43, 0x61, 0xFF, 0xFF, 0xFF, 0x72, 0x43, 0x61, 0xFF, 0xFF, 0xFF, 0x72, 0x43, 0x61, 0xFF},
  {0xFF, 0x74, 0x61, 0x7A, 0xFF, 0xFF, 0xFF, 0x74, 0x61, 0x7A, 0xFF, 0xFF, 0xFF, 0x74, 0x61, 0x7A, 0xFF},
};

void EllipsisView::drawRect(KDContext * ctx, KDRect rect) const {
  /* Draw the ellipsis vertically and horizontally centered in the view.
   * The heightCenter is the coordinate of the vertical middle of the view. That
   * way, (heightCenter-switchHalfHeight) indicates the top the ellipsis. */
  KDCoordinate widthCenter = bounds().width()/2;
  KDCoordinate ellipsisHalfWidth = k_ellipsisWidth/2;
  KDCoordinate heightCenter =  bounds().height()/2;
  KDCoordinate ellipsisHalfHeight = k_ellipsisHeight/2;
  KDRect frame(widthCenter - ellipsisHalfWidth, heightCenter - ellipsisHalfHeight, k_ellipsisWidth, k_ellipsisHeight);
  KDColor ellipsisWorkingBuffer[EllipsisView::k_ellipsisWidth*EllipsisView::k_ellipsisHeight];
  ctx->blendRectWithMask(frame, Palette::PrimaryText, (const uint8_t *)ellipsisMask, ellipsisWorkingBuffer);
}

KDSize EllipsisView::minimalSizeForOptimalDisplay() const {
  return KDSize(k_ellipsisWidth, k_ellipsisHeight);
}
