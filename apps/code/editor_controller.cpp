#include "editor_controller.h"
#include "menu_controller.h"
#include "script_parameter_controller.h"
#include "app.h"
#include <escher/metric.h>
#include <ion.h>
#include <string.h>
#include <stdint.h>
#include "../global_preferences.h"
#include <apps/apps_container.h>

using namespace Shared;

namespace Code {

EditorController::EditorController(MenuController * menuController, App * pythonDelegate) :
  ViewController(nullptr),
  m_editorView(this, pythonDelegate),
  m_script(Ion::Storage::Record()),
  m_scriptIndex(-1),
  m_menuController(menuController)
{
  m_editorView.setTextAreaDelegates(this, this);
}

void EditorController::setScript(Script script, int scriptIndex) {
  m_script = script;
  m_scriptIndex = scriptIndex;

  /* We edit the script directly in the storage buffer. We thus put all the
   * storage available space at the end of the current edited script and we set
   * its size.
   *
   * |****|****|m_script|****|**********|¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨|
   *                                          available space
   * is transformed to:
   *
   * |****|****|m_script|¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨|****|**********|
   *                          available space
   *
   * */

  Ion::Storage::sharedStorage()->putAvailableSpaceAtEndOfRecord(m_script);
  m_editorView.setText(const_cast<char *>(m_script.content()), m_script.contentSize());
}

void EditorController::willExitApp() {
  cleanStorageEmptySpace();
}

// TODO: this should be done in textAreaDidFinishEditing maybe??
bool EditorController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::Back || event == Ion::Events::Home || event == Ion::Events::USBEnumeration) {
    /* Exit the edition on USB enumeration, because the storage needs to be in a
     * "clean" state (with all records packed at the beginning of the storage) */
    cleanStorageEmptySpace();
    stackController()->pop();
    return event != Ion::Events::Home && event != Ion::Events::USBEnumeration;
  }
  return false;
}

void EditorController::didBecomeFirstResponder() {
  Container::activeApp()->setFirstResponder(&m_editorView);
}

void EditorController::viewWillAppear() {
  ViewController::viewWillAppear();
  m_editorView.loadSyntaxHighlighter();
  // Try to restore saved cursor position from a companion storage record
  // named "<basename>.cursor". If not present or invalid, fall back to
  // placing the cursor at the end of the text.
  if (!m_script.isNull()) {
    const char * scriptFullName = m_script.fullName();
    if (scriptFullName != nullptr) {
      const char * dot = strchr(scriptFullName, '.');
      size_t baseLen = dot ? (size_t)(dot - scriptFullName) : strlen(scriptFullName);
      char cursorFullName[Script::k_defaultScriptNameMaxSize + 1 + 6 + 1];
      memcpy(cursorFullName, scriptFullName, baseLen);
      cursorFullName[baseLen] = '.';
      memcpy(cursorFullName + baseLen + 1, "cursor", sizeof("cursor"));

      Ion::Storage::Record cursorRecord = Ion::Storage::sharedStorage()->recordNamed(cursorFullName);
      if (!cursorRecord.isNull()) {
        Ion::Storage::Record::Data d = cursorRecord.value();
        if (d.size >= sizeof(uint16_t)) {
          const uint8_t * buf = static_cast<const uint8_t *>(d.buffer);
          uint16_t pos = (uint16_t)(buf[0] | (buf[1] << 8));
          size_t textLen = strlen(m_editorView.text());
          if (pos > textLen) {
            pos = (uint16_t)textLen;
          }
          m_editorView.setCursorLocation(m_editorView.text() + pos);
          return;
        }
      }
    }
  }
  // Fallback: cursor at end of text
  m_editorView.setCursorLocation(m_editorView.text() + strlen(m_editorView.text()));
}

void EditorController::viewDidDisappear() {
  // Persist cursor position in a companion storage record named
  // "<basename>.cursor". The record contains a 2-byte little-endian
  // uint16_t offset (bytes) from the start of the script content.
  if (!m_script.isNull()) {
    const char * scriptFullName = m_script.fullName();
    if (scriptFullName != nullptr) {
      const char * dot = strchr(scriptFullName, '.');
      size_t baseLen = dot ? (size_t)(dot - scriptFullName) : strlen(scriptFullName);
      char cursorFullName[Script::k_defaultScriptNameMaxSize + 1 + 6 + 1];
      memcpy(cursorFullName, scriptFullName, baseLen);
      cursorFullName[baseLen] = '.';
      memcpy(cursorFullName + baseLen + 1, "cursor", sizeof("cursor"));

      const char * text = m_editorView.text();
      const char * cursor = m_editorView.cursorLocation();
      uint16_t pos = 0;
      if (text != nullptr && cursor != nullptr && cursor >= text) {
        size_t offset = (size_t)(cursor - text);
        pos = offset > UINT16_MAX ? UINT16_MAX : (uint16_t)offset;
      }
      Ion::Storage::Record::ErrorStatus status = Ion::Storage::sharedStorage()->createRecordWithFullName(cursorFullName, &pos, sizeof(pos));
      if (status == Ion::Storage::Record::ErrorStatus::NameTaken) {
        Ion::Storage::Record r = Ion::Storage::sharedStorage()->recordNamed(cursorFullName);
        Ion::Storage::Record::Data data{ &pos, sizeof(pos) };
        r.setValue(data);
      }
    }
  }
  m_editorView.resetSelection();
  m_menuController->scriptContentEditionDidFinish();
}

void EditorController::textAreaDidReceiveNoneXNTEvent() {
  AppsContainer::sharedAppsContainer()->resetXNT();
}

bool EditorController::textAreaDidReceiveEvent(TextArea * textArea, Ion::Events::Event event) {
  if (App::app()->textInputDidReceiveEvent(textArea, event)) {
    return true;
  }
  if (event == Ion::Events::EXE) {
    textArea->handleEventWithText("\n", true, false);
    return true;
  }

  if (event == Ion::Events::Backspace && textArea->selectionIsEmpty()) {
    /* If the cursor is on the left of the text of a line, backspace one
     * indentation space at a time. */
    const char * text = textArea->text();
    const char * cursorLocation = textArea->cursorLocation();
    const char * firstNonSpace = UTF8Helper::NotCodePointSearch(text, ' ', true, cursorLocation);
    assert(firstNonSpace >= text);
    bool cursorIsPrecededOnTheLineBySpacesOnly = false;
    size_t numberOfSpaces = cursorLocation - firstNonSpace;
    if (UTF8Helper::CodePointIs(firstNonSpace, '\n')) {
      cursorIsPrecededOnTheLineBySpacesOnly = true;
      numberOfSpaces -= UTF8Decoder::CharSizeOfCodePoint('\n');
    } else if (firstNonSpace == text) {
      cursorIsPrecededOnTheLineBySpacesOnly = true;
    }
    numberOfSpaces = numberOfSpaces / UTF8Decoder::CharSizeOfCodePoint(' ');
    if (cursorIsPrecededOnTheLineBySpacesOnly && numberOfSpaces >= TextArea::k_indentationSpaces) {
      for (int i = 0; i < TextArea::k_indentationSpaces; i++) {
        textArea->removePreviousGlyph();
      }
      return true;
    }
  } else if (event == Ion::Events::Space) {
    /* If the cursor is on the left of the text of a line, a space triggers an
     * indentation. */
    const char * text = textArea->text();
    const char * firstNonSpace = UTF8Helper::NotCodePointSearch(text, ' ', true, textArea->cursorLocation());
    assert(firstNonSpace >= text);
    if (UTF8Helper::CodePointIs(firstNonSpace, '\n')) {
      assert(UTF8Decoder::CharSizeOfCodePoint(' ') == 1);
      char indentationBuffer[TextArea::k_indentationSpaces+1];
      for (int i = 0; i < TextArea::k_indentationSpaces; i++) {
        indentationBuffer[i] = ' ';
      }
      indentationBuffer[TextArea::k_indentationSpaces] = 0;
      textArea->handleEventWithText(indentationBuffer);
      return true;
    }
  }
  return false;
}

VariableBoxController * EditorController::variableBoxForInputEventHandler(InputEventHandler * textInput) {
  VariableBoxController * varBox = App::app()->variableBoxController();
  /* If the editor should be autocompleting an identifier, the variable box has
   * already been loaded. We check shouldAutocomplete and not isAutocompleting,
   * because the autocompletion result might be empty. */
  const char * beginningOfAutocompletion = nullptr;
  const char * cursor = nullptr;
  PythonTextArea::AutocompletionType autocompType = m_editorView.autocompletionType(&beginningOfAutocompletion, &cursor);
  if (autocompType == PythonTextArea::AutocompletionType::NoIdentifier) {
    varBox->loadFunctionsAndVariables(m_scriptIndex, nullptr, 0);
  } else if (autocompType == PythonTextArea::AutocompletionType::MiddleOfIdentifier) {
    varBox->empty();
  } else {
    assert(autocompType == PythonTextArea::AutocompletionType::EndOfIdentifier);
    assert(beginningOfAutocompletion != nullptr && cursor != nullptr);
    assert(cursor > beginningOfAutocompletion);
    varBox->loadFunctionsAndVariables(m_scriptIndex, beginningOfAutocompletion, cursor - beginningOfAutocompletion);
  }
  varBox->setTitle(I18n::Message::Autocomplete);
  varBox->setDisplaySubtitles(true);
  return varBox;
}

StackViewController * EditorController::stackController() {
  return static_cast<StackViewController *>(parentResponder());
}

void EditorController::cleanStorageEmptySpace() {
  if (m_script.isNull() || !Ion::Storage::sharedStorage()->hasRecord(m_script)) {
    return;
  }
  Ion::Storage::Record::Data scriptValue = m_script.value();
  Ion::Storage::sharedStorage()->getAvailableSpaceFromEndOfRecord(
      m_script,
      scriptValue.size - Script::StatusSize() - (strlen(m_script.content()) + 1)); // TODO optimize number of script fetches
}


}
