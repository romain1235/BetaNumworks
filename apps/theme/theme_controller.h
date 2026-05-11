#ifndef APPS_THEME_CONTROLLER_H
#define APPS_THEME_CONTROLLER_H

#include <escher.h>
#include <apps/i18n.h>
#include "theme_loader.h"
#include <kandinsky/size.h>

/**
 * ThemeController shows a scrollable list:
 *   [0]  Default  (compiled-in palette)
 *   [1…] <file.theme>  (files on external flash)
 *
 * It is used both by OnBoarding (via Delegate callback) and by Settings (stack push).
 */
class ThemeController : public ViewController,
                        public SimpleListViewDataSource,
                        public SelectableTableViewDataSource {
public:
  static constexpr size_t k_maxNameLen = 42;

  class Delegate {
  public:
    virtual void themeControllerDidSelectTheme(ThemeController * controller) = 0;
  };

  ThemeController(Responder * parentResponder, Delegate * delegate = nullptr);

  // ViewController
  View * view() override { return &m_contentView; }
  const char * title() override;
  void viewWillAppear() override;
  void viewDidDisappear() override;
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;

  // SimpleListViewDataSource
  int numberOfRows() const override;
  KDCoordinate cellHeight() override;
  HighlightCell * reusableCell(int index) override;
  int reusableCellCount() const override;
  void willDisplayCellForIndex(HighlightCell * cell, int index) override;

  TELEMETRY_ID("Theme");

private:
  void applySelectedTheme(int row);

  // Wrapper view so that when displayed as a modal the controller fills the
  // full screen (minimalSizeForOptimalDisplay returns KDSizeZero). When the
  // controller has a Delegate (used by OnBoarding) we display a large title
  // at the top, like we do for language/country selection.
  class ContentView : public View {
  public:
    ContentView(ThemeController * controller, SelectableTableView * tableView);
    KDSize minimalSizeForOptimalDisplay() const override { return KDSizeZero; }
  private:
    int numberOfSubviews() const override;
    View * subviewAtIndex(int) override;
    void layoutSubviews(bool force = false) override;
    ThemeController * m_controller;
    SelectableTableView * m_tableView;
    MessageTextView m_titleMessage;
  };

  static constexpr int k_maxThemeFiles = 20;
  static constexpr int k_cellCount     = 6;

  SelectableTableView m_tableView;
  ContentView         m_contentView;
  MessageTableCell<> m_cells[k_cellCount];
  Delegate * m_delegate;

  int  m_themeCount;
  int  m_activeRow;  // row index of the currently active theme (0 = default)
  char m_themeNames[k_maxThemeFiles][k_maxNameLen];
  // Per-cell name buffers (MessageTextView stores a pointer, not a copy)
  char m_nameBufs[k_cellCount][k_maxNameLen + 4];
};

#endif
