#ifndef ESCHER_EXPRESSION_TABLE_CELL_WITH_POINTER_H
#define ESCHER_EXPRESSION_TABLE_CELL_WITH_POINTER_H

#include <escher/expression_table_cell.h>
#include <escher/i18n.h>
#include <escher/message_text_view.h>

class ExpressionTableCellWithPointer : public ExpressionTableCell {
public:
  ExpressionTableCellWithPointer(Responder * responder = nullptr, I18n::Message accessoryMessage = (I18n::Message)0, Layout layout = Layout::HorizontalRightOverlap);
  View * accessoryView() const override;
  void setHighlighted(bool highlight) override;
  void setAccessoryMessage(I18n::Message messageBody);
  void setAccessoryTextColor(KDColor color) { m_accessoryView.setTextColor(color); }
  void setAccessoryHorizontalAlignment(float alignment) { m_accessoryView.setAlignment(alignment, 0.5f); }
private:
  MessageTextView m_accessoryView;
};

#endif
