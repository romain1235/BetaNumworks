#include "button_with_separator.h"

ButtonWithSeparator::ButtonWithSeparator(Responder * parentResponder, I18n::Message message, Invocation invocation) :
  Button(parentResponder, message, invocation, KDFont::LargeFont, Palette::ButtonText),
  m_squareCorners(KDSquareCornerAll),
  m_borderBackgroundColor(Palette::ListCellBackground)
{
}

void ButtonWithSeparator::configureListAppearance(uint8_t squareCorners, KDColor borderBackgroundColor) {
  if (m_squareCorners != squareCorners || m_borderBackgroundColor != borderBackgroundColor) {
    m_squareCorners = squareCorners;
    m_borderBackgroundColor = borderBackgroundColor;
    layoutSubviews(true);
    reloadCell();
  }
}

void ButtonWithSeparator::setHighlighted(bool highlight) {
  if (m_squareCorners == KDSquareCornerAll) {
    Button::setHighlighted(highlight);
    return;
  }
  HighlightCell::setHighlighted(highlight);
}

int ButtonWithSeparator::numberOfSubviews() const {
  return m_squareCorners == KDSquareCornerAll ? 1 : 0;
}

void ButtonWithSeparator::drawRect(KDContext * ctx, KDRect rect) const {
  if (m_squareCorners == KDSquareCornerAll) {
    drawRectangular(ctx);
    return;
  }
  if (isStandalone()) {
    drawStandaloneRounded(ctx);
    return;
  }
  drawConnectedRounded(ctx);
}

void ButtonWithSeparator::drawRectangular(KDContext * ctx) const {
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  ctx->fillRect(KDRect(0, 0, width, k_lineThickness), Palette::ListCellBorder);
  ctx->fillRect(KDRect(0, k_lineThickness, width, k_margin-k_lineThickness), Palette::BackgroundApps);
  ctx->fillRect(KDRect(0, k_margin, width, k_lineThickness), Palette::ListCellBorder);
  ctx->fillRect(KDRect(0, k_margin+k_lineThickness, k_lineThickness, height-k_margin), Palette::ListCellBorder);
  ctx->fillRect(KDRect(width-k_lineThickness, k_lineThickness+k_margin, k_lineThickness, height-k_margin), Palette::ListCellBorder);
  ctx->fillRect(KDRect(0, height-3*k_lineThickness, width, k_lineThickness), Palette::ButtonBorderOut);
  ctx->fillRect(KDRect(0, height-2*k_lineThickness, width, k_lineThickness), Palette::ListCellBorder);
  ctx->fillRect(KDRect(k_lineThickness, height-k_lineThickness, width-2*k_lineThickness, k_lineThickness), Palette::ButtonShadow);
}

void ButtonWithSeparator::drawStandaloneRounded(KDContext * ctx) const {
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  ctx->fillRect(KDRect(0, 0, width, height), m_borderBackgroundColor);
  KDColor buttonBg = isHighlighted() ? highlightedBackgroundColor() : Palette::ButtonBackground;
  KDPoint previousOrigin = ctx->origin();
  ctx->setOrigin(previousOrigin.translatedBy(KDPoint(0, k_topGap)));
  drawBorderOfRect(ctx, KDRect(0, 0, width, height - k_topGap), Palette::ListCellBorder, m_borderBackgroundColor, 0, buttonBg);
  ctx->setOrigin(previousOrigin);
  drawButtonLabel(ctx);
}

void ButtonWithSeparator::drawConnectedRounded(KDContext * ctx) const {
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  ctx->fillRect(KDRect(0, 0, width, height), m_borderBackgroundColor);
  KDColor buttonBg = isHighlighted() ? highlightedBackgroundColor() : Palette::ButtonBackground;
  KDPoint previousOrigin = ctx->origin();
  ctx->setOrigin(previousOrigin.translatedBy(KDPoint(0, k_topGap)));
  drawBorderOfRect(ctx, KDRect(0, 0, width, height - k_topGap), Palette::ListCellBorder, m_borderBackgroundColor, m_squareCorners, buttonBg);
  ctx->setOrigin(previousOrigin);
  drawButtonLabel(ctx);
}

void ButtonWithSeparator::drawButtonLabel(KDContext * ctx) const {
  const char * text = m_messageTextView.text();
  if (text == nullptr) {
    return;
  }
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  KDCoordinate labelTop = k_topGap + k_separatorThickness;
  KDCoordinate labelHeight = height - k_topGap - 2 * k_separatorThickness;
  KDSize textSize = KDFont::LargeFont->stringSize(text);
  KDPoint origin(
    (width - textSize.width()) / 2,
    labelTop + (labelHeight - textSize.height()) / 2);
  KDColor backgroundColor = isHighlighted() ? highlightedBackgroundColor() : Palette::ButtonBackground;
  ctx->drawString(text, origin, KDFont::LargeFont, Palette::ButtonText, backgroundColor);
}

void ButtonWithSeparator::layoutSubviews(bool force) {
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  if (m_squareCorners == KDSquareCornerAll) {
    m_messageTextView.setFrame(KDRect(
      k_lineThickness,
      k_margin + k_lineThickness,
      width - 2 * k_lineThickness,
      height - k_margin - 3 * k_lineThickness),
    force);
    return;
  }
  m_messageTextView.setFrame(KDRect(
    k_separatorThickness,
    k_topGap + k_separatorThickness,
    width - 2 * k_separatorThickness,
    height - k_topGap - 2 * k_separatorThickness),
  force);
}
