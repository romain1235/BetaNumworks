#ifndef ESCHER_HIGHLIGHT_CELL_H
#define ESCHER_HIGHLIGHT_CELL_H

#include <escher/view.h>
#include <escher/responder.h>
#include <kandinsky/color.h>
#include <poincare/layout.h>

class HighlightCell : public View {
public:
  HighlightCell();
  virtual void setHighlighted(bool highlight);
  bool isHighlighted() const { return m_highlighted; }
  virtual void reloadCell();
  virtual Responder * responder() {
    return nullptr;
  }
  virtual const char * text() const {
    return nullptr;
  }
  virtual Poincare::Layout layout() const {
    return Poincare::Layout();
  }
  virtual void configureListAppearance(uint8_t squareCorners, KDColor borderBackgroundColor);
protected:
  bool m_highlighted;
};

#endif
