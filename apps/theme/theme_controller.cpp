#include "theme_controller.h"
#include "theme_loader.h"
#include <apps/i18n.h>
#include <apps/apps_container.h>
#include <escher/metric.h>
#include <string.h>

ThemeController::ThemeController(Responder * parentResponder, Delegate * delegate) :
  ViewController(parentResponder),
  m_tableView(this, this, this),
  m_contentView(this, &m_tableView),
  m_delegate(delegate),
  m_themeCount(0),
  m_activeRow(0)
{
  for (int i = 0; i < k_cellCount; i++) {
    m_cells[i].setMessageFont(KDFont::LargeFont);
  }
  memset(m_themeNames, 0, sizeof(m_themeNames));
  memset(m_nameBufs, 0, sizeof(m_nameBufs));
}

ThemeController::ContentView::ContentView(ThemeController * controller, SelectableTableView * tableView) :
  m_controller(controller), m_tableView(tableView), m_titleMessage(KDFont::LargeFont, I18n::Message::Theme) {
  m_titleMessage.setBackgroundColor(Palette::BackgroundHard);
  m_titleMessage.setAlignment(0.5f, 0.5f);
}

int ThemeController::ContentView::numberOfSubviews() const {
  return 1 + (m_controller->m_delegate ? 1 : 0);
}

View * ThemeController::ContentView::subviewAtIndex(int i) {
  // If title is displayed, it is the second subview.
  if (m_controller->m_delegate && i == 1) {
    return &m_titleMessage;
  }
  return m_tableView;
}

void ThemeController::ContentView::layoutSubviews(bool force) {
  if (m_controller->m_delegate) {
    KDCoordinate titleHeight = m_titleMessage.font()->glyphSize().height();
    m_titleMessage.setFrame(KDRect(0, Metric::CommonTopMargin, bounds().width(), titleHeight), force);
    m_tableView->setFrame(KDRect(0, Metric::CommonTopMargin + titleHeight, bounds().width(), bounds().height() - (Metric::CommonTopMargin + titleHeight)), force);
  } else {
    m_tableView->setFrame(bounds(), force);
  }
}
const char * ThemeController::title() {
  return I18n::translate(I18n::Message::Theme);
}

void ThemeController::viewWillAppear() {
  // Refresh the list of .theme files each time the view appears
  m_themeCount = 0;
  int nb = ThemeLoader::numberOfThemeFiles();
  for (int i = 0; i < nb && m_themeCount < k_maxThemeFiles; i++) {
    if (ThemeLoader::themeFileAtIndex(i, m_themeNames[m_themeCount], k_maxNameLen)) {
      m_themeCount++;
    }
  }
  // Determine which row is currently active
  char currentName[ThemeLoader::k_maxThemeNameLength];
  m_activeRow = 0;  // default unless a stored theme matches
  if (ThemeLoader::readPersistedThemeName(currentName, sizeof(currentName)) && currentName[0] != '\0') {
    for (int i = 0; i < m_themeCount; i++) {
      if (strcmp(m_themeNames[i], currentName) == 0) {
        m_activeRow = i + 1;
        break;
      }
    }
  }
  selectCellAtLocation(0, m_activeRow);
  m_tableView.reloadData();
}

void ThemeController::didBecomeFirstResponder() {
  Container::activeApp()->setFirstResponder(&m_tableView);
}

void ThemeController::viewDidDisappear() {
  m_tableView.deselectTable();
}

bool ThemeController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    applySelectedTheme(selectedRow());
    return true;
  }
  if (event == Ion::Events::Left || event == Ion::Events::Back) {
    // In settings: pop; in onboarding: Back = select Default and proceed
    if (m_delegate == nullptr) {
      static_cast<StackViewController *>(parentResponder())->pop();
    } else {
      applySelectedTheme(0);
    }
    return true;
  }
  return false;
}

int ThemeController::numberOfRows() const {
  return 1 + m_themeCount;  // "Default" + one per .theme file
}

KDCoordinate ThemeController::cellHeight() {
  return Metric::ParameterCellHeight;
}

HighlightCell * ThemeController::reusableCell(int index) {
  return &m_cells[index];
}

int ThemeController::reusableCellCount() const {
  return k_cellCount;
}

void ThemeController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  MessageTableCell<> * c = static_cast<MessageTableCell<> *>(cell);
  // Use a per-cell buffer — MessageTextView stores a pointer, not a copy.
  int cellIdx = (int)(c - m_cells);
  if (cellIdx < 0 || cellIdx >= k_cellCount) cellIdx = 0;
  char * buf = m_nameBufs[cellIdx];
  const size_t bufSize = k_maxNameLen + 4;
  MessageTextView * label = static_cast<MessageTextView *>(c->labelView());

  const char * prefix = (index == m_activeRow) ? "> " : "";

  if (index == 0) {
    const char * translated = I18n::translate(I18n::Message::DefaultTheme);
    size_t plen = strlen(prefix);
    size_t tlen = strlen(translated);
    if (plen + tlen >= bufSize) tlen = bufSize - plen - 1;
    memcpy(buf, prefix, plen);
    memcpy(buf + plen, translated, tlen);
    buf[plen + tlen] = '\0';
    label->setText(buf);
  } else {
    const char * name = m_themeNames[index - 1];
    size_t nameLen = strlen(name);
    if (nameLen > 6 && strcmp(name + nameLen - 6, ".theme") == 0) {
      nameLen -= 6;
    }
    size_t plen = strlen(prefix);
    if (plen + nameLen >= bufSize) nameLen = bufSize - plen - 1;
    memcpy(buf, prefix, plen);
    memcpy(buf + plen, name, nameLen);
    buf[plen + nameLen] = '\0';
    label->setText(buf);
  }
}

void ThemeController::applySelectedTheme(int row) {
  if (row == 0) {
    ThemeLoader::resetToDefault();
    ThemeLoader::persistThemeName("");
  } else {
    const char * name = m_themeNames[row - 1];
    ThemeLoader::loadFromFlash(name);
    ThemeLoader::persistThemeName(name);
  }

  // Refresh the toolbar immediately with new palette colors
  AppsContainer::sharedAppsContainer()->reloadTitleBarView();
  // Redraw entire window so icons and colors update everywhere
  AppsContainer::sharedAppsContainer()->redrawWindow(true);

  // Update active row indicator and reload list
  m_activeRow = row;
  m_tableView.reloadData();

  if (m_delegate) {
    m_delegate->themeControllerDidSelectTheme(this);
  } else {
    static_cast<StackViewController *>(parentResponder())->pop();
  }
}
