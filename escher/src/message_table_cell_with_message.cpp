#include <escher/message_table_cell_with_message.h>
#include <escher/palette.h>
#include <string.h>

template <class T>
MessageTableCellWithMessage<T>::MessageTableCellWithMessage(I18n::Message message, TableCell::Layout layout) :
  MessageTableCell<T>(message, KDFont::SmallFont, layout),
  m_accessoryView(KDFont::SmallFont, (I18n::Message)0, 0.0f, 0.5f)
{
  /* For the Adaptive layout the description must stay left-aligned: when it
   * falls back to a vertical layout its frame spans the whole width and it has
   * to start under the label. In the horizontal case its frame is sized to its
   * content and pushed against the right edge, so it appears right-aligned
   * anyway. Only the fixed horizontal layouts need an explicit right alignment. */
  if (layout == TableCell::Layout::HorizontalLeftOverlap || layout == TableCell::Layout::HorizontalRightOverlap) {
    m_accessoryView.setAlignment(1.0f, 0.5f);
  }
}

template <class T>
void MessageTableCellWithMessage<T>::setAccessoryMessage(I18n::Message textBody) {
  m_accessoryView.setMessage(textBody);
  this->reloadCell();
}

template <class T>
View * MessageTableCellWithMessage<T>::accessoryView() const {
  if (strlen(m_accessoryView.text()) == 0) {
    return nullptr;
  }
  return (View *)&m_accessoryView;
}

template <class T>
void MessageTableCellWithMessage<T>::setHighlighted(bool highlight) {
  MessageTableCell<T>::setHighlighted(highlight);
  KDColor backgroundColor = this->isHighlighted()? Palette::ListCellBackgroundSelected : Palette::ListCellBackground;
  m_accessoryView.setBackgroundColor(backgroundColor);
}

template <class T>
void MessageTableCellWithMessage<T>::setTextColor(KDColor color) {
  m_accessoryView.setTextColor(color);
  MessageTableCell<T>::setTextColor(color);
}

template <class T>
void MessageTableCellWithMessage<T>::setAccessoryTextColor(KDColor color) {
  m_accessoryView.setTextColor(color);
}

template class MessageTableCellWithMessage<MessageTextView>;
template class MessageTableCellWithMessage<SlideableMessageTextView>;
