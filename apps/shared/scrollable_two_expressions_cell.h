#ifndef SHARED_SCROLLABLE_TWO_EXPRESSIONS_CELL_H
#define SHARED_SCROLLABLE_TWO_EXPRESSIONS_CELL_H

#include <escher.h>
#include "scrollable_multiple_expressions_view.h"

namespace Shared {

class ScrollableTwoExpressionsCell : public ::EvenOddCell, public Responder {
public:
  ScrollableTwoExpressionsCell(Responder * parentResponder = nullptr);
  void setLayouts(Poincare::Layout approximateLayout, Poincare::Layout exactLayout);
  void setEqualMessage(I18n::Message equalSignMessage) {
    return m_view.setEqualMessage(equalSignMessage);
  }
  void setHighlighted(bool highlight) override;
  void setEven(bool even) override;
  void reloadScroll();
  Responder * responder() override {
    return this;
  }
  Poincare::Layout layout() const override { return m_view.layout(); }
  void didBecomeFirstResponder() override;
  void reinitSelection();
private:
  class SolverScrollableTwoExpressionsView : public AbstractScrollableMultipleExpressionsView {
  public:
    SolverScrollableTwoExpressionsView(Responder * parentResponder) : AbstractScrollableMultipleExpressionsView(parentResponder, &m_contentCell) {
      setMargins(
          Metric::CommonSmallMargin,
          Metric::CommonLargeMargin,
          Metric::CommonSmallMargin,
          Metric::CommonLargeMargin
      );
    }
  private:
    class ContentCell : public AbstractScrollableMultipleExpressionsView::ContentCell {
    public:
      KDColor backgroundColor() const override {
        return m_even ? Palette::BackgroundHard : Palette::BackgroundApps;
      }
    };
    ContentCell * contentCell() override { return &m_contentCell; }
    const ContentCell * constContentCell() const override { return &m_contentCell; }
    ContentCell m_contentCell;
  };

  int numberOfSubviews() const override;
  View * subviewAtIndex(int index) override;
  void layoutSubviews(bool force = false) override;
  SolverScrollableTwoExpressionsView m_view;
};

}

#endif
