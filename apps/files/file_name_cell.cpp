#include "file_name_cell.h"
#include <assert.h>
#include <escher.h>

namespace Files {

void FileNameCell::setEven(bool even) {
  EvenOddCell::setEven(even);
  m_textField.setBackgroundColor(backgroundColor());
}

void FileNameCell::setHighlighted(bool highlight) {
  EvenOddCell::setHighlighted(highlight);
  m_textField.setBackgroundColor(backgroundColor());
}

const char * FileNameCell::text() const {
  if (!m_textField.isEditing()) {
    return m_textField.text();
  }
  return nullptr;
}

KDSize FileNameCell::minimalSizeForOptimalDisplay() const {
  return m_textField.minimalSizeForOptimalDisplay();
}

void FileNameCell::didBecomeFirstResponder() {
  Container::activeApp()->setFirstResponder(&m_textField);
}

void FileNameCell::layoutSubviews(bool force) {
  KDRect cellBounds = bounds();
  m_textField.setFrame(KDRect(cellBounds.x() + k_leftMargin,
        cellBounds.y(),
        cellBounds.width() - k_leftMargin,
        cellBounds.height()),
    force);
}

}
