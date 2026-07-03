#ifndef SHARED_BUTTON_WITH_SEPARATOR_H
#define SHARED_BUTTON_WITH_SEPARATOR_H

#include <escher.h>
#include <escher/bordered.h>

class ButtonWithSeparator : public Button, public Bordered {
public:
  constexpr static KDCoordinate k_topGap = 10;
  ButtonWithSeparator(Responder * parentResponder, I18n::Message textBody, Invocation invocation);
  void drawRect(KDContext * ctx, KDRect rect) const override;
  void setHighlighted(bool highlight) override;
  void configureListAppearance(uint8_t squareCorners, KDColor borderBackgroundColor) override;
private:
  constexpr static KDCoordinate k_margin = 5;
  constexpr static KDCoordinate k_lineThickness = 1;
  int numberOfSubviews() const override;
  void layoutSubviews(bool force = false) override;
  void drawRectangular(KDContext * ctx) const;
  void drawStandaloneRounded(KDContext * ctx) const;
  void drawConnectedRounded(KDContext * ctx) const;
  void drawButtonLabel(KDContext * ctx) const;
  bool isStandalone() const { return m_squareCorners == 0; }
  uint8_t m_squareCorners;
  KDColor m_borderBackgroundColor;
};

#endif
