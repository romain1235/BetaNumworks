#ifndef CODE_PYTHON_TEXT_AREA_H
#define CODE_PYTHON_TEXT_AREA_H

#include <escher/text_area.h>
#include <stdint.h>

namespace Code {

class App;

class PythonTextArea : public TextArea {
public:
  enum class AutocompletionType : uint8_t {
    EndOfIdentifier,
    MiddleOfIdentifier,
    NoIdentifier
  };
  PythonTextArea(Responder * parentResponder, App * pythonDelegate, const KDFont * font) :
    TextArea(parentResponder, &m_contentView, font),
    m_contentView(pythonDelegate, font),
    m_autocompletionResultIndex(0)
  {
  }
  void loadSyntaxHighlighter() { m_contentView.loadSyntaxHighlighter(); }
  void unloadSyntaxHighlighter() { m_contentView.unloadSyntaxHighlighter(); }
  bool handleEvent(Ion::Events::Event event) override;
  bool handleEventWithText(const char * text, bool indentation = false, bool forceCursorRightOfText = false, bool shouldRemoveLastCharacter = false) override;
  /* autocompletionType returns:
   * - EndOfIdentifier if there is currently autocompletion, or if the cursor is
   *   at the end of an identifier,
   * - MiddleOfIdentifier is the cursor is in the middle of an identifier,
   * - No identifier otherwise.
   * The autocompletionLocation can be provided with autocompletionLocation, or
   * retrieved with autocompletionLocationBeginning and autocompletionLocationEnd. */
  AutocompletionType autocompletionType(const char * autocompletionLocation = nullptr, const char ** autocompletionLocationBeginning = nullptr, const char ** autocompletionLocationEnd = nullptr) const;
  bool isAutocompleting() const { return m_contentView.isAutocompleting(); }
protected:
  class ContentView : public TextArea::ContentView {
  public:
    constexpr static int kInvalidEstimateWindowRadius = 512;

    ContentView(App * pythonDelegate, const KDFont * font) :
      TextArea::ContentView(font),
      m_pythonDelegate(pythonDelegate),
      m_autocomplete(false),
      m_autocompletionEnd(nullptr),
      m_delimiterColoringCacheIsValid(false),
      m_invalidOpeningsCount(0),
      m_invalidClosingsCount(0),
      m_lineDepthCount(0)
    {
    }
    App * pythonDelegate() { return m_pythonDelegate; }
    void setAutocompleting(bool autocomplete) { m_autocomplete = autocomplete; }
    bool isAutocompleting() const { return m_autocomplete; }
    const char * autocompletionEnd() const { assert(m_autocomplete); return m_autocompletionEnd; }
    void setAutocompletionEnd(const char * end) { m_autocompletionEnd = end; }
    const char * textToAutocomplete() const;
    void loadSyntaxHighlighter();
    void unloadSyntaxHighlighter();
    void invalidateDelimiterColoringCache();
    /* Light-weight accessors for the delimiter-coloring cache.
     * These do NOT force a full re-lex: prefer using them only when the
     * cache is already valid (checked via delimiterColoringCacheIsValid()). */
    bool delimiterColoringCacheIsValid() const;
    bool hasInvalidClosingAfter(const char * position, int maxDistance = 256) const;
    /* Estimate the change in number of invalid delimiters when inserting
     * `insertedText` at `position`. The function lexes a local window around
     * `position` (cost controlled by `windowRadius`) and returns
     * `invalid_after - invalid_before`. Positive means worse (more invalids),
     * negative means better (fewer invalids). */
    int estimateInvalidDeltaForInsertion(const char * position, const char * insertedText, int insertedLen, int windowRadius = kInvalidEstimateWindowRadius) const;
    /* Estimate the change in number of invalid delimiters when deleting
     * `deletionLen` characters starting at `position`. Returns
     * `invalid_after - invalid_before`. Positive means worse, negative
     * means better. */
    int estimateInvalidDeltaForDeletion(const char * position, int deletionLen, int windowRadius = kInvalidEstimateWindowRadius) const;
    void clearRect(KDContext * ctx, KDRect rect) const override;
    void drawLine(KDContext * ctx, int line, const char * text, size_t length, int fromColumn, int toColumn, const char * selectionStart, const char * selectionEnd) const override;
    KDRect dirtyRectFromPosition(const char * position, bool includeFollowingLines) const override;
    void insertTextAtLocation(const char * text, char * location, int textLength = -1) override;
    bool removePreviousGlyph() override;
    bool removeEndOfLine() override;
  private:
    using DelimiterOffset = uint16_t;
    using DelimiterDepth = uint8_t;

    void updateDelimiterColoringCache() const;
    int delimiterDepthAtLine(int line) const;
    bool isInvalidOpeningDelimiter(const char * position) const;
    bool isInvalidClosingDelimiter(const char * position) const;
    char * invalidEstimateScratchBuffer() const;

    constexpr static int kInvalidDelimitersCapacity = 256;
    constexpr static int kLineDepthCapacity = 512;
    /* Reuses m_invalidOpenings + m_invalidClosings as a scratch text buffer. */
    constexpr static int kInvalidEstimateBufferSize = 2 * kInvalidDelimitersCapacity * sizeof(DelimiterOffset);

    App * m_pythonDelegate;
    bool m_autocomplete;
    const char * m_autocompletionEnd;
    mutable bool m_delimiterColoringCacheIsValid;
    mutable int m_invalidOpeningsCount;
    mutable DelimiterOffset m_invalidOpenings[kInvalidDelimitersCapacity];
    mutable int m_invalidClosingsCount;
    mutable DelimiterOffset m_invalidClosings[kInvalidDelimitersCapacity];
    mutable int m_lineDepthCount;
    mutable DelimiterDepth m_lineStartDelimiterDepths[kLineDepthCapacity];

  };
private:
  void removeAutocompletion();
  void removeAutocompletionText(); // Just removes the suggested text, not the autocompletion mode
  void addAutocompletion();
  bool addAutocompletionTextAtIndex(int nextIndex, int * currentIndexToUpdate = nullptr); // Assumes the var box is already loaded
  void cycleAutocompletion(bool downwards);
  void acceptAutocompletion(bool moveCursorToEndOfAutocompletion);
  const ContentView * nonEditableContentView() const override { return &m_contentView; }
  ContentView m_contentView;
  int m_autocompletionResultIndex;
 
};

}

#endif
