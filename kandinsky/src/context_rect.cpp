#include <kandinsky/context.h>
#include <assert.h>
#include <algorithm>
#include <math.h>

static void fillAntiAliasedQuarterCircle(KDContext * ctx, KDPoint center, KDCoordinate r, KDColor color, const KDColor * backgroundColor, bool top, bool left) {
  KDCoordinate minX = left ? center.x() - r - 1 : center.x();
  KDCoordinate maxX = left ? center.x() : center.x() + r + 1;
  KDCoordinate minY = top ? center.y() - r - 1 : center.y();
  KDCoordinate maxY = top ? center.y() : center.y() + r + 1;
  KDCoordinate width = maxX - minX + 1;
  KDCoordinate height = maxY - minY + 1;
  uint8_t mask[width * height];
  KDColor workingBuffer[width * height];
  for (KDCoordinate j = 0; j < height; j++) {
    for (KDCoordinate i = 0; i < width; i++) {
      KDCoordinate px = minX + i;
      KDCoordinate py = minY + j;
      float dx = (px + 0.5f) - center.x();
      float dy = (py + 0.5f) - center.y();
      if (left ? (dx > 0.f) : (dx < 0.f)) {
        mask[j * width + i] = 0xFF;
        continue;
      }
      if (top ? (dy > 0.f) : (dy < 0.f)) {
        mask[j * width + i] = 0xFF;
        continue;
      }
      float dist = sqrt(dx * dx + dy * dy);
      float coverage = r + 0.5f - dist;
      if (coverage <= 0.f) {
        mask[j * width + i] = 0xFF;
        continue;
      }
      // In KDColor::blend, mask=0 means full color and mask=255 means full background.
      mask[j * width + i] = coverage >= 1.f ? 0 : (uint8_t)((1.f - coverage) * 255.f);
    }
  }
  KDRect cornerRect(minX, minY, width, height);
  if (backgroundColor == nullptr) {
    ctx->blendRectWithMask(cornerRect, color, mask, workingBuffer);
    return;
  }
  ctx->getPixels(cornerRect, workingBuffer);
  for (KDCoordinate j = 0; j < height; j++) {
    for (KDCoordinate i = 0; i < width; i++) {
      uint8_t maskValue = mask[j * width + i];
      if (maskValue == 0) {
        workingBuffer[j * width + i] = color;
      } else if (maskValue < 0xFF) {
        workingBuffer[j * width + i] = KDColor::blend(*backgroundColor, color, maskValue);
      }
    }
  }
  ctx->fillRectWithPixels(cornerRect, workingBuffer, nullptr);
}

static void fillRoundedRectBody(KDContext * ctx, KDRect rect, KDCoordinate r, KDColor color, const KDColor * backgroundColor, uint8_t squareCorners) {
  KDCoordinate x = rect.x();
  KDCoordinate y = rect.y();
  KDCoordinate w = rect.width();
  KDCoordinate h = rect.height();
  bool roundTL = (squareCorners & KDSquareCornerTopLeft) == 0;
  bool roundTR = (squareCorners & KDSquareCornerTopRight) == 0;
  bool roundBL = (squareCorners & KDSquareCornerBottomLeft) == 0;
  bool roundBR = (squareCorners & KDSquareCornerBottomRight) == 0;
  KDCoordinate leftInset = (roundTL || roundBL) ? r : 0;
  KDCoordinate rightInset = (roundTR || roundBR) ? r : 0;
  KDCoordinate topInset = (roundTL || roundTR) ? r : 0;
  KDCoordinate bottomInset = (roundBL || roundBR) ? r : 0;
  ctx->fillRect(KDRect(x + leftInset, y, w - leftInset - rightInset, h), color);
  if (leftInset > 0) {
    ctx->fillRect(KDRect(x, y + topInset, leftInset, h - topInset - bottomInset), color);
  }
  if (rightInset > 0) {
    ctx->fillRect(KDRect(x + w - rightInset, y + topInset, rightInset, h - topInset - bottomInset), color);
  }
  if (roundTL) {
    fillAntiAliasedQuarterCircle(ctx, KDPoint(x + r, y + r), r, color, backgroundColor, true, true);
  }
  if (roundTR) {
    fillAntiAliasedQuarterCircle(ctx, KDPoint(x + w - r, y + r), r, color, backgroundColor, true, false);
  }
  if (roundBL) {
    fillAntiAliasedQuarterCircle(ctx, KDPoint(x + r, y + h - r), r, color, backgroundColor, false, true);
  }
  if (roundBR) {
    fillAntiAliasedQuarterCircle(ctx, KDPoint(x + w - r, y + h - r), r, color, backgroundColor, false, false);
  }
}

KDRect KDContext::absoluteFillRect(KDRect rect) {
  return rect.translatedBy(m_origin).intersectedWith(m_clippingRect);
}

void KDContext::fillRect(KDRect rect, KDColor color) {
  KDRect absoluteRect = absoluteFillRect(rect);
  if (absoluteRect.isEmpty()) {
    return;
  }
  pushRectUniform(absoluteRect, color);
}

void KDContext::fillRoundedRect(KDRect rect, KDCoordinate radius, KDColor color) {
  if (rect.isEmpty()) {
    return;
  }
  KDCoordinate r = radius;
  KDCoordinate maxRadius = std::min(rect.width(), rect.height()) / 2;
  if (r > maxRadius) {
    r = maxRadius;
  }
  if (r <= 0) {
    fillRect(rect, color);
    return;
  }
  fillRoundedRectBody(this, rect, r, color, nullptr, 0);
}

void KDContext::fillRoundedRect(KDRect rect, KDCoordinate radius, KDColor color, KDColor backgroundColor) {
  if (rect.isEmpty()) {
    return;
  }
  KDCoordinate r = radius;
  KDCoordinate maxRadius = std::min(rect.width(), rect.height()) / 2;
  if (r > maxRadius) {
    r = maxRadius;
  }
  if (r <= 0) {
    fillRect(rect, color);
    return;
  }
  fillRoundedRectBody(this, rect, r, color, &backgroundColor, 0);
}

void KDContext::fillRoundedRect(KDRect rect, KDCoordinate radius, KDColor color, uint8_t squareCorners) {
  if (rect.isEmpty()) {
    return;
  }
  KDCoordinate r = radius;
  KDCoordinate maxRadius = std::min(rect.width(), rect.height()) / 2;
  if (r > maxRadius) {
    r = maxRadius;
  }
  if (r <= 0) {
    fillRect(rect, color);
    return;
  }
  fillRoundedRectBody(this, rect, r, color, nullptr, squareCorners);
}

void KDContext::fillRoundedRect(KDRect rect, KDCoordinate radius, KDColor color, KDColor backgroundColor, uint8_t squareCorners) {
  if (rect.isEmpty()) {
    return;
  }
  KDCoordinate r = radius;
  KDCoordinate maxRadius = std::min(rect.width(), rect.height()) / 2;
  if (r > maxRadius) {
    r = maxRadius;
  }
  if (r <= 0) {
    fillRect(rect, color);
    return;
  }
  fillRoundedRectBody(this, rect, r, color, &backgroundColor, squareCorners);
}

/* Note: we support the case where workingBuffer IS equal to pixels */
void KDContext::fillRectWithPixels(KDRect rect, const KDColor * pixels, KDColor * workingBuffer) {
  KDRect absoluteRect = absoluteFillRect(rect);

  if (absoluteRect.isEmpty()) {
    return;
  }

  /* Caution:
   * The absoluteRect may have a SMALLER size than the original rect because it
   * has been clipped. Therefore we cannot assume that the mask can be read as a
   * continuous area. */

  if (absoluteRect.width() == rect.width() && absoluteRect.height() == rect.height()) {
    pushRect(absoluteRect, pixels);
    return;
  }

  KDCoordinate startingI = m_clippingRect.x() - rect.translatedBy(m_origin).x();
  KDCoordinate startingJ = m_clippingRect.y() - rect.translatedBy(m_origin).y();
  startingI = startingI > 0 ? startingI : 0;
  startingJ = startingJ > 0 ? startingJ : 0;

  /* If the rect has indeed been clipped, we only want to push the correct
   * discontinuous extract of pixels. We want also to minimize calls to pushRect
   * (time consuming). If a working buffer is available, we can fill it by
   * concatenating extracted rows of 'pixels' to call pushRect only once on the
   * absoluteRect. However, if we do not have a working buffer, we push row by
   * row extracts of 'pixels' calling pushRect multiple times. */

  if (workingBuffer == nullptr) {
    for (KDCoordinate j=0; j<absoluteRect.height(); j++) {
      KDRect absoluteRow = KDRect(absoluteRect.x(), absoluteRect.y()+j, absoluteRect.width(), 1);
      KDColor * rowPixels = (KDColor *)pixels+startingI+rect.width()*(startingJ+j);
      pushRect(absoluteRow, rowPixels);
    }
  } else {
    for (KDCoordinate j=0; j<absoluteRect.height(); j++) {
      for (KDCoordinate i=0; i<absoluteRect.width(); i++) {
        workingBuffer[i+absoluteRect.width()*j] = pixels[startingI+i+rect.width()*(startingJ+j)];
      }
    }
    pushRect(absoluteRect, workingBuffer);
  }
}

// Mask's size must be rect.size
// WorkingBuffer, same deal
// TODO: should we avoid pullRect by giving a 'memory' working buffer?
void KDContext::blendRectWithMask(KDRect rect, KDColor color, const uint8_t * mask, KDColor * workingBuffer) {
  KDRect absoluteRect = absoluteFillRect(rect);

  /* Caution:
   * The absoluteRect may have a SMALLER size than the original rect because it
   * has been clipped. Therefore we cannot assume that the mask can be read as a
   * continuous area. */

  pullRect(absoluteRect, workingBuffer);
  KDCoordinate startingI = m_clippingRect.x() - rect.translatedBy(m_origin).x();
  KDCoordinate startingJ = m_clippingRect.y() - rect.translatedBy(m_origin).y();
  startingI = startingI > 0 ? startingI : 0;
  startingJ = startingJ > 0 ? startingJ : 0;
  for (KDCoordinate j=0; j<absoluteRect.height(); j++) {
    for (KDCoordinate i=0; i<absoluteRect.width(); i++) {
      KDColor * currentPixelAddress = workingBuffer + i + absoluteRect.width()*j;
      const uint8_t * currentMaskAddress = mask + i + startingI + rect.width()*(j + startingJ);
      *currentPixelAddress = KDColor::blend(*currentPixelAddress, color, *currentMaskAddress);
      //*currentPixelAddress = KDColorBlend(*currentPixelAddress, color, *currentMaskAddress);
    }
  }
  pushRect(absoluteRect, workingBuffer);
}

void KDContext::strokeRect(KDRect rect, KDColor color) {
  fillRect(KDRect(rect.origin(), rect.width(), 1), color);
  fillRect(KDRect(KDPoint(rect.x(), rect.bottom()), rect.width(), 1), color);
  fillRect(KDRect(rect.origin(), 1, rect.height()), color);
  fillRect(KDRect(KDPoint(rect.right(), rect.y()), 1, rect.height()), color);
}

