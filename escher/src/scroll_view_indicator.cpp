#include <escher/scroll_view_indicator.h>
#include <escher/metric.h>
#include <escher/palette.h>
extern "C" {
#include <assert.h>
}
#include <cmath>

ScrollViewIndicator::ScrollViewIndicator() :
  View(),
  m_color(Palette::ScrollBarForeground),
  m_margin(Metric::CommonTopMargin)
{
}

ScrollViewBar::ScrollViewBar() :
  ScrollViewIndicator(),
  m_offset(0),
  m_visibleLength(0),
  m_trackColor(Palette::ScrollBarBackground)
{
}

bool ScrollViewBar::update(KDCoordinate totalContentLength, KDCoordinate contentOffset, KDCoordinate visibleContentLength) {
  float offset = contentOffset;
  float visibleLength = visibleContentLength;
  offset = offset / totalContentLength;
  visibleLength = visibleLength / totalContentLength;
  if (m_offset != offset || m_visibleLength != visibleLength) {
    m_offset = offset;
    m_visibleLength = visibleLength;
    markRectAsDirty(bounds());
  }
  return visible();
}

void ScrollViewHorizontalBar::drawRect(KDContext * ctx, KDRect rect) const {
  if (!visible()) {
    return;
  }
  int y = (m_frame.height() - k_indicatorThickness)/2;
  int visibleLength = std::ceil(m_visibleLength*totalLength());
  int totalLengthValue = totalLength();
  int indicatorX = m_margin+m_offset*totalLengthValue;
  ctx->fillRect(
    KDRect(
      m_margin, y+1,
      1, k_indicatorThickness-2
    ),
    m_trackColor
  );
  ctx->fillRect(
    KDRect(
      m_margin+totalLengthValue-1, y+1,
      1, k_indicatorThickness-2
    ),
    m_trackColor
  );
  ctx->fillRect(
    KDRect(
      m_margin+1, y,
      totalLengthValue-2, k_indicatorThickness
    ),
    m_trackColor
  );
  ctx->fillRect(
    KDRect(
      indicatorX, y+1,
      1, k_indicatorThickness-2
    ),
    m_color
  );
  ctx->fillRect(
    KDRect(
      indicatorX+visibleLength-1, y+1,
      1, k_indicatorThickness-2
    ),
    m_color
  );
  ctx->fillRect(
    KDRect(
      indicatorX+1, y,
      visibleLength-2, k_indicatorThickness
    ),
    m_color
  );
}

void ScrollViewVerticalBar::drawRect(KDContext * ctx, KDRect rect) const {
  if (!visible()) {
    return;
  }
  int x = (m_frame.width() - k_indicatorThickness)/2;
  int visibleLength = std::ceil(m_visibleLength*totalLength());
  int totalLengthValue = totalLength();
  int indicatorY = m_margin+m_offset*totalLengthValue;
  ctx->fillRect(
    KDRect(
      x+1, m_margin,
      k_indicatorThickness-2, 1
    ),
    m_trackColor
  );
  ctx->fillRect(
    KDRect(
      x+1, m_margin+totalLengthValue-1,
      k_indicatorThickness-2, 1
    ),
    m_trackColor
  );
  ctx->fillRect(
    KDRect(
      x, m_margin+1,
      k_indicatorThickness, totalLengthValue-2
    ),
    m_trackColor
  );
  ctx->fillRect(
    KDRect(
      x+1, indicatorY,
      k_indicatorThickness-2, 1
    ),
    m_color
  ); 
  ctx->fillRect(
    KDRect(
      x+1, indicatorY+visibleLength-1,
      k_indicatorThickness-2, 1
    ),
    m_color
  ); 
  ctx->fillRect(
    KDRect(
      x, indicatorY+1,
      k_indicatorThickness, visibleLength-2
    ),
    m_color
  );
}

ScrollViewArrow::ScrollViewArrow(Side side) :
  m_visible(false),
  m_arrow(side)
{
}

bool ScrollViewArrow::update(bool visible) {
  if (m_visible != visible) {
    markRectAsDirty(bounds());
  }
  m_visible = visible;
  return visible;
}

void ScrollViewArrow::drawRect(KDContext * ctx, KDRect rect) const {
  ctx->fillRect(bounds(), m_backgroundColor);
  KDSize arrowSize = KDFont::LargeFont->glyphSize();
  const KDPoint arrowAlign = KDPoint(
    (m_arrow == Top || m_arrow == Bottom) * (m_frame.width() - arrowSize.width()) / 2,
    (m_arrow == Left || m_arrow == Right) * (m_frame.height() - arrowSize.height()) / 2
  );
  char arrowString[2] = {m_arrow, 0}; // TODO Change when code points
  ctx->drawString(arrowString, arrowAlign, KDFont::LargeFont, m_color, m_backgroundColor, m_visible);
}

#if ESCHER_VIEW_LOGGING
const char * ScrollViewIndicator::className() const {
  return "ScrollViewIndicator";
}

void ScrollViewIndicator::logAttributes(std::ostream &os) const {
  View::logAttributes(os);
  os << " offset=\"" << m_offset << "\"";
  os << " visibleLength=\"" << m_visibleLength << "\"";
}
#endif
