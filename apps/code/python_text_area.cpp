#include "python_text_area.h"
#include "app.h"
#include <escher/palette.h>
#include <ion/keyboard.h>
#include <ion/unicode/utf8_helper.h>
#include <python/port/port.h>
#include "../global_preferences.h"

extern "C" {
#include "py/nlr.h"
#include "py/lexer.h"
}
#include <stdlib.h>
#include <algorithm>
#include <string.h>

namespace Code {

// These are macros rather than const variables so that they read the
// (potentially runtime-overridden) Palette values without introducing
// any global constructors (which are not supported on this platform).
#define CommentColor            (Palette::CodeComment)
#define NumberColor             (Palette::CodeNumber)
#define KeywordColor            (Palette::CodeKeyword)
#define OperatorColor           (Palette::CodeOperator)
#define StringColor             (Palette::CodeString)
#define BackgroundColor         (Palette::CodeBackground)
#define HighlightColor          (Palette::CodeBackgroundSelected)
#define InvalidParenthesisColor (Palette::invalid_parenthese)
#define IndentGuideColor        (Palette::CodeComment)
// AutocompleteColor is a fixed color, not palette-driven
constexpr KDColor AutocompleteColor = KDColor::RGB24(0xC6C6C6);

static inline KDColor ParenthesisColorForDepth(int depth) {
  int normalizedDepth = depth % 3;
  if (normalizedDepth < 0) {
    normalizedDepth += 3;
  }
  switch (normalizedDepth) {
    case 0: return Palette::parenthese_1;
    case 1: return Palette::parenthese_2;
    default: return Palette::parenthese_3;
  }
}

static inline int NextDelimiterDepth(int depth) {
  return (depth + 1) % 3;
}

static inline int PreviousDelimiterDepth(int depth) {
  return (depth + 2) % 3;
}

static inline bool IsOpeningDelimiter(mp_token_kind_t tokenKind) {
  return tokenKind == MP_TOKEN_DEL_PAREN_OPEN
      || tokenKind == MP_TOKEN_DEL_BRACKET_OPEN
      || tokenKind == MP_TOKEN_DEL_BRACE_OPEN;
}

static inline bool IsClosingDelimiter(mp_token_kind_t tokenKind) {
  return tokenKind == MP_TOKEN_DEL_PAREN_CLOSE
      || tokenKind == MP_TOKEN_DEL_BRACKET_CLOSE
      || tokenKind == MP_TOKEN_DEL_BRACE_CLOSE;
}

static inline bool TextContainsDelimiter(const char * text) {
  for (const char * c = text; *c != 0; c++) {
    if (*c == '(' || *c == ')' || *c == '[' || *c == ']' || *c == '{' || *c == '}') {
      return true;
    }
  }
  return false;
}

static inline bool IsOpeningDelimiterChar(char c) {
  return c == '(' || c == '[' || c == '{';
}

static inline bool IsClosingDelimiterChar(char c) {
  return c == ')' || c == ']' || c == '}';
}

static inline char MatchingClosingDelimiterChar(char openingDelimiter) {
  if (openingDelimiter == '(') {
    return ')';
  }
  if (openingDelimiter == '[') {
    return ']';
  }
  if (openingDelimiter == '{') {
    return '}';
  }
  return 0;
}

static inline int OffsetAfterInsertion(int offset, int insertionOffset, int insertionLength) {
  return offset >= insertionOffset ? offset + insertionLength : offset;
}

static inline int OffsetAfterDeletion(int offset, int deletionOffset, int deletionLength) {
  if (offset <= deletionOffset) {
    return offset;
  }
  if (offset < deletionOffset + deletionLength) {
    return deletionOffset;
  }
  return offset - deletionLength;
}

static int SelectedLineStartOffsets(const char * text, int selectionStartOffset, int selectionEndOffset, int * lineStartOffsets, int maxLineCount) {
  if (selectionStartOffset < 0 || selectionEndOffset <= selectionStartOffset || maxLineCount <= 0) {
    return 0;
  }

  int blockStartOffset = selectionStartOffset;
  while (blockStartOffset > 0 && text[blockStartOffset - 1] != '\n') {
    blockStartOffset--;
  }

  int lineCount = 0;
  const char * lineStart = text + blockStartOffset;
  const char * selectionEnd = text + selectionEndOffset;
  while (lineStart < selectionEnd && lineCount < maxLineCount) {
    lineStartOffsets[lineCount++] = lineStart - text;
    const char * nextLine = UTF8Helper::CodePointSearch(lineStart, '\n');
    if (UTF8Helper::CodePointIs(nextLine, UCodePointNull)) {
      break;
    }
    lineStart = nextLine + 1;
  }

  return lineCount;
}

static inline char OpeningDelimiterFor(mp_token_kind_t tokenKind) {
  if (tokenKind == MP_TOKEN_DEL_PAREN_OPEN || tokenKind == MP_TOKEN_DEL_PAREN_CLOSE) {
    return '(';
  }
  if (tokenKind == MP_TOKEN_DEL_BRACKET_OPEN || tokenKind == MP_TOKEN_DEL_BRACKET_CLOSE) {
    return '[';
  }
  return '{';
}

static inline int DelimiterTypeIndex(mp_token_kind_t tokenKind) {
  if (tokenKind == MP_TOKEN_DEL_PAREN_OPEN || tokenKind == MP_TOKEN_DEL_PAREN_CLOSE) {
    return 0;
  }
  if (tokenKind == MP_TOKEN_DEL_BRACKET_OPEN || tokenKind == MP_TOKEN_DEL_BRACKET_CLOSE) {
    return 1;
  }
  if (tokenKind == MP_TOKEN_DEL_BRACE_OPEN || tokenKind == MP_TOKEN_DEL_BRACE_CLOSE) {
    return 2;
  }
  return -1;
}

static inline bool OffsetInList(uint16_t offset, const uint16_t * list, int listLength) {
  for (int i = 0; i < listLength; i++) {
    if (list[i] == offset) {
      return true;
    }
  }
  return false;
}

static inline uint16_t OffsetForPosition(const char * base, const char * position) {
  size_t offset = position - base;
  return offset > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(offset);
}

bool isItalic(mp_token_kind_t tokenKind) {
  if (!GlobalPreferences::sharedGlobalPreferences()->syntaxhighlighting()) {
    return false;
  }
  if (tokenKind == MP_TOKEN_STRING) {
    return true;
  }
  return false;
}

static inline KDColor TokenColor(mp_token_kind_t tokenKind) {
  if (!GlobalPreferences::sharedGlobalPreferences()->syntaxhighlighting()) {
    return Palette::CodeText;
  }
  if (tokenKind == MP_TOKEN_STRING) {
    return StringColor;
  }
  if (tokenKind == MP_TOKEN_INTEGER || tokenKind == MP_TOKEN_FLOAT_OR_IMAG) {
    return NumberColor;
  }
  static_assert(MP_TOKEN_ELLIPSIS + 1 == MP_TOKEN_KW_FALSE
      && MP_TOKEN_KW_FALSE      + 1 == MP_TOKEN_KW_NONE
      && MP_TOKEN_KW_NONE       + 1 == MP_TOKEN_KW_TRUE
      && MP_TOKEN_KW_TRUE       + 1 == MP_TOKEN_KW___DEBUG__
      && MP_TOKEN_KW___DEBUG__  + 1 == MP_TOKEN_KW_AND
      && MP_TOKEN_KW_AND        + 1 == MP_TOKEN_KW_AS
      && MP_TOKEN_KW_AS         + 1 == MP_TOKEN_KW_ASSERT
      /* Here there are keywords that depend on MICROPY_PY_ASYNC_AWAIT, we do
       * not test them */
      && MP_TOKEN_KW_BREAK      + 1 == MP_TOKEN_KW_CLASS
      && MP_TOKEN_KW_CLASS      + 1 == MP_TOKEN_KW_CONTINUE
      && MP_TOKEN_KW_CONTINUE   + 1 == MP_TOKEN_KW_DEF
      && MP_TOKEN_KW_DEF        + 1 == MP_TOKEN_KW_DEL
      && MP_TOKEN_KW_DEL        + 1 == MP_TOKEN_KW_ELIF
      && MP_TOKEN_KW_ELIF       + 1 == MP_TOKEN_KW_ELSE
      && MP_TOKEN_KW_ELSE       + 1 == MP_TOKEN_KW_EXCEPT
      && MP_TOKEN_KW_EXCEPT     + 1 == MP_TOKEN_KW_FINALLY
      && MP_TOKEN_KW_FINALLY    + 1 == MP_TOKEN_KW_FOR
      && MP_TOKEN_KW_FOR        + 1 == MP_TOKEN_KW_FROM
      && MP_TOKEN_KW_FROM       + 1 == MP_TOKEN_KW_GLOBAL
      && MP_TOKEN_KW_GLOBAL     + 1 == MP_TOKEN_KW_IF
      && MP_TOKEN_KW_IF         + 1 == MP_TOKEN_KW_IMPORT
      && MP_TOKEN_KW_IMPORT     + 1 == MP_TOKEN_KW_IN
      && MP_TOKEN_KW_IN         + 1 == MP_TOKEN_KW_IS
      && MP_TOKEN_KW_IS         + 1 == MP_TOKEN_KW_LAMBDA
      && MP_TOKEN_KW_LAMBDA     + 1 == MP_TOKEN_KW_NONLOCAL
      && MP_TOKEN_KW_NONLOCAL   + 1 == MP_TOKEN_KW_NOT
      && MP_TOKEN_KW_NOT        + 1 == MP_TOKEN_KW_OR
      && MP_TOKEN_KW_OR         + 1 == MP_TOKEN_KW_PASS
      && MP_TOKEN_KW_PASS       + 1 == MP_TOKEN_KW_RAISE
      && MP_TOKEN_KW_RAISE      + 1 == MP_TOKEN_KW_RETURN
      && MP_TOKEN_KW_RETURN     + 1 == MP_TOKEN_KW_TRY
      && MP_TOKEN_KW_TRY        + 1 == MP_TOKEN_KW_WHILE
      && MP_TOKEN_KW_WHILE      + 1 == MP_TOKEN_KW_WITH
      && MP_TOKEN_KW_WITH       + 1 == MP_TOKEN_KW_YIELD
      && MP_TOKEN_KW_YIELD      + 1 == MP_TOKEN_OP_ASSIGN
      && MP_TOKEN_OP_ASSIGN     + 1 == MP_TOKEN_OP_TILDE,
    "MP_TOKEN order changed, so Code::PythonTextArea::TokenColor might need to change too.");
  if (tokenKind >= MP_TOKEN_KW_FALSE && tokenKind <= MP_TOKEN_KW_YIELD) {
    return KeywordColor;
  }
  static_assert(MP_TOKEN_OP_TILDE       + 1 == MP_TOKEN_OP_LESS
      && MP_TOKEN_OP_LESS               + 1 == MP_TOKEN_OP_MORE
      && MP_TOKEN_OP_MORE               + 1 == MP_TOKEN_OP_DBL_EQUAL
      && MP_TOKEN_OP_DBL_EQUAL          + 1 == MP_TOKEN_OP_LESS_EQUAL
      && MP_TOKEN_OP_LESS_EQUAL         + 1 == MP_TOKEN_OP_MORE_EQUAL
      && MP_TOKEN_OP_MORE_EQUAL         + 1 == MP_TOKEN_OP_NOT_EQUAL
      && MP_TOKEN_OP_NOT_EQUAL          + 1 == MP_TOKEN_OP_PIPE
      && MP_TOKEN_OP_PIPE               + 1 == MP_TOKEN_OP_CARET
      && MP_TOKEN_OP_CARET              + 1 == MP_TOKEN_OP_AMPERSAND
      && MP_TOKEN_OP_AMPERSAND          + 1 == MP_TOKEN_OP_DBL_LESS
      && MP_TOKEN_OP_DBL_LESS           + 1 == MP_TOKEN_OP_DBL_MORE
      && MP_TOKEN_OP_DBL_MORE           + 1 == MP_TOKEN_OP_PLUS
      && MP_TOKEN_OP_PLUS               + 1 == MP_TOKEN_OP_MINUS
      && MP_TOKEN_OP_MINUS              + 1 == MP_TOKEN_OP_STAR
      && MP_TOKEN_OP_STAR               + 1 == MP_TOKEN_OP_AT
      && MP_TOKEN_OP_AT                 + 1 == MP_TOKEN_OP_DBL_SLASH
      && MP_TOKEN_OP_DBL_SLASH          + 1 == MP_TOKEN_OP_SLASH
      && MP_TOKEN_OP_SLASH              + 1 == MP_TOKEN_OP_PERCENT
      && MP_TOKEN_OP_PERCENT            + 1 == MP_TOKEN_OP_DBL_STAR
      && MP_TOKEN_OP_DBL_STAR           + 1 == MP_TOKEN_DEL_PIPE_EQUAL
      && MP_TOKEN_DEL_PIPE_EQUAL        + 1 == MP_TOKEN_DEL_CARET_EQUAL
      && MP_TOKEN_DEL_CARET_EQUAL       + 1 == MP_TOKEN_DEL_AMPERSAND_EQUAL
      && MP_TOKEN_DEL_AMPERSAND_EQUAL   + 1 == MP_TOKEN_DEL_DBL_LESS_EQUAL
      && MP_TOKEN_DEL_DBL_LESS_EQUAL    + 1 == MP_TOKEN_DEL_DBL_MORE_EQUAL
      && MP_TOKEN_DEL_DBL_MORE_EQUAL    + 1 == MP_TOKEN_DEL_PLUS_EQUAL
      && MP_TOKEN_DEL_PLUS_EQUAL        + 1 == MP_TOKEN_DEL_MINUS_EQUAL
      && MP_TOKEN_DEL_MINUS_EQUAL       + 1 == MP_TOKEN_DEL_STAR_EQUAL
      && MP_TOKEN_DEL_STAR_EQUAL        + 1 == MP_TOKEN_DEL_AT_EQUAL
      && MP_TOKEN_DEL_AT_EQUAL          + 1 == MP_TOKEN_DEL_DBL_SLASH_EQUAL
      && MP_TOKEN_DEL_DBL_SLASH_EQUAL   + 1 == MP_TOKEN_DEL_SLASH_EQUAL
      && MP_TOKEN_DEL_SLASH_EQUAL       + 1 == MP_TOKEN_DEL_PERCENT_EQUAL
      && MP_TOKEN_DEL_PERCENT_EQUAL     + 1 == MP_TOKEN_DEL_DBL_STAR_EQUAL
      && MP_TOKEN_DEL_DBL_STAR_EQUAL    + 1 == MP_TOKEN_DEL_PAREN_OPEN
      && MP_TOKEN_DEL_PAREN_OPEN        + 1 == MP_TOKEN_DEL_PAREN_CLOSE
      && MP_TOKEN_DEL_PAREN_CLOSE       + 1 == MP_TOKEN_DEL_BRACKET_OPEN
      && MP_TOKEN_DEL_BRACKET_OPEN      + 1 == MP_TOKEN_DEL_BRACKET_CLOSE
      && MP_TOKEN_DEL_BRACKET_CLOSE     + 1 == MP_TOKEN_DEL_BRACE_OPEN
      && MP_TOKEN_DEL_BRACE_OPEN        + 1 == MP_TOKEN_DEL_BRACE_CLOSE
      && MP_TOKEN_DEL_BRACE_CLOSE       + 1 == MP_TOKEN_DEL_COMMA
      && MP_TOKEN_DEL_COMMA             + 1 == MP_TOKEN_DEL_COLON
      && MP_TOKEN_DEL_COLON             + 1 == MP_TOKEN_DEL_PERIOD
      && MP_TOKEN_DEL_PERIOD            + 1 == MP_TOKEN_DEL_SEMICOLON
      && MP_TOKEN_DEL_SEMICOLON         + 1 == MP_TOKEN_DEL_EQUAL
      && MP_TOKEN_DEL_EQUAL             + 1 == MP_TOKEN_DEL_MINUS_MORE,
    "MP_TOKEN order changed, so Code::PythonTextArea::TokenColor might need to change too.");

  if ((tokenKind >= MP_TOKEN_OP_TILDE && tokenKind <= MP_TOKEN_DEL_DBL_STAR_EQUAL)
      || tokenKind == MP_TOKEN_DEL_EQUAL
      || tokenKind == MP_TOKEN_DEL_MINUS_MORE
      || tokenKind == MP_TOKEN_OP_ASSIGN)
  {
    return OperatorColor;
  }
  return Palette::CodeText;
}

static inline size_t TokenLength(mp_lexer_t * lex, const char * tokenPosition) {
  /* The lexer stores the beginning of the current token and of the next token,
   * so we just use that. */
  if (lex->line > 1) {
    /* The next token is on the next line, so we cannot just make the difference
     * of the columns. */
    return UTF8Helper::CodePointSearch(tokenPosition, '\n') - tokenPosition;
  }
  return lex->column - lex->tok_column;
}

PythonTextArea::AutocompletionType PythonTextArea::autocompletionType(const char * autocompletionLocation, const char ** autocompletionLocationBeginning, const char ** autocompletionLocationEnd) const {
  const char * location = autocompletionLocation != nullptr ? autocompletionLocation : cursorLocation();
  const char * beginningOfToken = nullptr;

  /* If there is already autocompleting, the cursor must be at the end of an
   * identifier. Trying to compute autocompletionType will fail: because of the
   * autocompletion text, the cursor seems to be in the middle of an identifier. */
  AutocompletionType autocompleteType = isAutocompleting() ? AutocompletionType::EndOfIdentifier : AutocompletionType::NoIdentifier;
  if (autocompletionLocationBeginning == nullptr && autocompletionLocationEnd == nullptr) {
    return autocompleteType;
  }
  nlr_buf_t nlr;
  if (nlr_push(&nlr) == 0) {
    const char * firstNonSpace = UTF8Helper::BeginningOfWord(m_contentView.editedText(), location);
    mp_lexer_t * lex = mp_lexer_new_from_str_len(0, firstNonSpace, UTF8Helper::EndOfWord(location) - firstNonSpace, 0);

    const char * tokenStart;
    const char * tokenEnd;
    _mp_token_kind_t currentTokenKind = lex->tok_kind;

    while (currentTokenKind != MP_TOKEN_NEWLINE && currentTokenKind != MP_TOKEN_END && currentTokenKind != MP_TOKEN_FSTRING_RAW) {
      tokenStart = firstNonSpace + lex->tok_column - 1;
      tokenEnd = tokenStart + TokenLength(lex, tokenStart);

      if (location < tokenStart) {
        // The location for autocompletion is not in an identifier
        assert(autocompleteType == AutocompletionType::NoIdentifier);
        break;
      }
      if (location <= tokenEnd) {
        if (currentTokenKind == MP_TOKEN_NAME
            || (currentTokenKind >= MP_TOKEN_KW_FALSE
              && currentTokenKind <= MP_TOKEN_KW_YIELD))
        {
          /* The location for autocompletion is in the middle or at the end of
           * an identifier. */
          beginningOfToken = tokenStart;
          /* If autocompleteType is already EndOfIdentifier, we are
           * autocompleting, so we do not need to update autocompleteType. If we
           * recomputed autocompleteType now, we might wrongly think that it is
           * MiddleOfIdentifier because of the autocompetion text.
           * Example : fin|ally -> the lexer is at the end of "fin", but because
           * we are autocompleting with "ally", the lexer thinks the cursor is
           * in the middle of an identifier. */
          if (autocompleteType != AutocompletionType::EndOfIdentifier) {
            autocompleteType = location < tokenEnd ? AutocompletionType::MiddleOfIdentifier : AutocompletionType::EndOfIdentifier;
          }
        }
        break;
      }
      mp_lexer_to_next(lex);
      currentTokenKind = lex->tok_kind;
    }
    mp_lexer_free(lex);
    nlr_pop();
  }
  if (autocompletionLocationBeginning != nullptr) {
    *autocompletionLocationBeginning = beginningOfToken;
  }
  if (autocompletionLocationEnd != nullptr) {
    *autocompletionLocationEnd = location;
  }
  assert(!isAutocompleting() || autocompleteType == AutocompletionType::EndOfIdentifier);
  return autocompleteType;
}

const char * PythonTextArea::ContentView::textToAutocomplete() const {
  return UTF8Helper::BeginningOfWord(editedText(), cursorLocation());
}

void PythonTextArea::ContentView::loadSyntaxHighlighter() {
  invalidateDelimiterColoringCache();
  m_pythonDelegate->initPythonWithUser(this);
}

void PythonTextArea::ContentView::unloadSyntaxHighlighter() {
  m_pythonDelegate->deinitPython();
}

void PythonTextArea::ContentView::invalidateDelimiterColoringCache() {
  m_delimiterColoringCacheIsValid = false;
}

int PythonTextArea::ContentView::delimiterDepthAtLine(int line) const {
  updateDelimiterColoringCache();
  if (line < 0 || m_lineDepthCount <= 0) {
    return 0;
  }
  if (line < m_lineDepthCount) {
    return m_lineStartDelimiterDepths[line];
  }
  return m_lineStartDelimiterDepths[m_lineDepthCount - 1];
}

bool PythonTextArea::ContentView::isInvalidOpeningDelimiter(const char * position) const {
  updateDelimiterColoringCache();
  DelimiterOffset offset = OffsetForPosition(editedText(), position);
  return offset != UINT16_MAX && OffsetInList(offset, m_invalidOpenings, m_invalidOpeningsCount);
}

bool PythonTextArea::ContentView::isInvalidClosingDelimiter(const char * position) const {
  updateDelimiterColoringCache();
  DelimiterOffset offset = OffsetForPosition(editedText(), position);
  return offset != UINT16_MAX && OffsetInList(offset, m_invalidClosings, m_invalidClosingsCount);
}

bool PythonTextArea::ContentView::delimiterColoringCacheIsValid() const {
  return m_delimiterColoringCacheIsValid;
}

bool PythonTextArea::ContentView::hasInvalidClosingAfter(const char * position, int maxDistance) const {
  if (!m_delimiterColoringCacheIsValid) {
    return false;
  }
  DelimiterOffset offset = OffsetForPosition(editedText(), position);
  if (offset == UINT16_MAX) {
    return false;
  }
  for (int i = 0; i < m_invalidClosingsCount; i++) {
    DelimiterOffset off = m_invalidClosings[i];
    if (off > offset && off - offset <= maxDistance) {
      return true;
    }
  }
  return false;
}

int PythonTextArea::ContentView::estimateInvalidDeltaForInsertion(const char * position, const char * insertedText, int insertedLen, int windowRadius) const {
  const char * fullText = editedText();
  if (fullText == nullptr) {
    return 0;
  }
  const char * fullEnd = fullText + strlen(fullText);

  // Determine window bounds around position
  int back = 0;
  const char * windowStart = position;
  while (windowStart > fullText && back < windowRadius) {
    windowStart--;
    back++;
    if (*windowStart == '\n') { windowStart++; break; }
  }
  const char * windowEnd = position;
  int forward = 0;
  while (windowEnd < fullEnd && forward < windowRadius) {
    if (*windowEnd == '\n') { break; }
    windowEnd++;
    forward++;
  }

  size_t origLen = windowEnd - windowStart;
  // Build original buffer (C-style)
  char * origBuf = (char *)malloc(origLen + 1);
  if (origBuf == nullptr) {
    return 0;
  }
  memcpy(origBuf, windowStart, origLen);
  origBuf[origLen] = '\0';

  // Build new buffer with insertion
  size_t relPos = position - windowStart;
  size_t newLen = origLen + insertedLen;
  char * newBuf = (char *)malloc(newLen + 1);
  if (newBuf == nullptr) {
    free(origBuf);
    return 0;
  }
  memcpy(newBuf, origBuf, relPos);
  memcpy(newBuf + relPos, insertedText, insertedLen);
  memcpy(newBuf + relPos + insertedLen, origBuf + relPos, origLen - relPos);
  newBuf[newLen] = '\0';

  auto countInvalidsInBuffer = [](const char * buf, size_t len) -> int {
    const int kDelimiterTypeCount = 3;
    int openingDepth[kDelimiterTypeCount] = {0,0,0};
    int invalidClosings = 0;

    mp_lexer_t * lex = mp_lexer_new_from_str_len(0, buf, len, 0);
    const char * lineStart = buf;
    int currentLine = 1;
    while (lex->tok_kind != MP_TOKEN_END && lex->tok_kind != MP_TOKEN_FSTRING_RAW) {
      while (currentLine < lex->tok_line) {
        const char * nextLine = UTF8Helper::CodePointSearch(lineStart, '\n');
        if (UTF8Helper::CodePointIs(nextLine, UCodePointNull)) break;
        lineStart = nextLine + 1;
        currentLine++;
      }
      int delimiterType = DelimiterTypeIndex(lex->tok_kind);
      if (delimiterType >= 0) {
        if (IsOpeningDelimiter(lex->tok_kind)) {
          openingDepth[delimiterType]++;
        } else if (IsClosingDelimiter(lex->tok_kind)) {
          if (openingDepth[delimiterType] > 0) {
            openingDepth[delimiterType]--;
          } else {
            invalidClosings++;
          }
        }
      }
      mp_lexer_to_next(lex);
    }
    mp_lexer_free(lex);
    int remainingOpenings = openingDepth[0] + openingDepth[1] + openingDepth[2];
    return invalidClosings + remainingOpenings;
  };

  int before = countInvalidsInBuffer(origBuf, origLen);
  int after = countInvalidsInBuffer(newBuf, newLen);
  free(origBuf);
  free(newBuf);
  return after - before;
}

int PythonTextArea::ContentView::estimateInvalidDeltaForDeletion(const char * position, int deletionLen, int windowRadius) const {
  const char * fullText = editedText();
  if (fullText == nullptr || deletionLen <= 0) {
    return 0;
  }
  const char * fullEnd = fullText + strlen(fullText);

  // Determine window bounds around position
  int back = 0;
  const char * windowStart = position;
  while (windowStart > fullText && back < windowRadius) {
    windowStart--;
    back++;
    if (*windowStart == '\n') { windowStart++; break; }
  }
  const char * windowEnd = position;
  int forward = 0;
  while (windowEnd < fullEnd && forward < windowRadius) {
    if (*windowEnd == '\n') { break; }
    windowEnd++;
    forward++;
  }

  size_t origLen = windowEnd - windowStart;
  if (origLen == 0) {
    return 0;
  }

  // Build original buffer (C-style)
  char * origBuf = (char *)malloc(origLen + 1);
  if (origBuf == nullptr) {
    return 0;
  }
  memcpy(origBuf, windowStart, origLen);
  origBuf[origLen] = '\0';

  // Compute deletion relative to window
  size_t relPos = position - windowStart;
  int clampedDeletion = deletionLen;
  if ((size_t)clampedDeletion > origLen - relPos) {
    clampedDeletion = (int)(origLen - relPos);
  }
  if (clampedDeletion <= 0) {
    free(origBuf);
    return 0;
  }

  size_t newLen = origLen - clampedDeletion;
  char * newBuf = (char *)malloc(newLen + 1);
  if (newBuf == nullptr) {
    free(origBuf);
    return 0;
  }
  // Copy before deletion
  memcpy(newBuf, origBuf, relPos);
  // Copy after deletion
  memcpy(newBuf + relPos, origBuf + relPos + clampedDeletion, origLen - relPos - clampedDeletion);
  newBuf[newLen] = '\0';

  auto countInvalidsInBuffer = [](const char * buf, size_t len) -> int {
    const int kDelimiterTypeCount = 3;
    int openingDepth[kDelimiterTypeCount] = {0,0,0};
    int invalidClosings = 0;

    mp_lexer_t * lex = mp_lexer_new_from_str_len(0, buf, len, 0);
    const char * lineStart = buf;
    int currentLine = 1;
    while (lex->tok_kind != MP_TOKEN_END && lex->tok_kind != MP_TOKEN_FSTRING_RAW) {
      while (currentLine < lex->tok_line) {
        const char * nextLine = UTF8Helper::CodePointSearch(lineStart, '\n');
        if (UTF8Helper::CodePointIs(nextLine, UCodePointNull)) break;
        lineStart = nextLine + 1;
        currentLine++;
      }
      int delimiterType = DelimiterTypeIndex(lex->tok_kind);
      if (delimiterType >= 0) {
        if (IsOpeningDelimiter(lex->tok_kind)) {
          openingDepth[delimiterType]++;
        } else if (IsClosingDelimiter(lex->tok_kind)) {
          if (openingDepth[delimiterType] > 0) {
            openingDepth[delimiterType]--;
          } else {
            invalidClosings++;
          }
        }
      }
      mp_lexer_to_next(lex);
    }
    mp_lexer_free(lex);
    int remainingOpenings = openingDepth[0] + openingDepth[1] + openingDepth[2];
    return invalidClosings + remainingOpenings;
  };

  int before = countInvalidsInBuffer(origBuf, origLen);
  int after = countInvalidsInBuffer(newBuf, newLen);
  free(origBuf);
  free(newBuf);
  return after - before;
}

void PythonTextArea::ContentView::updateDelimiterColoringCache() const {
  if (m_delimiterColoringCacheIsValid) {
    return;
  }

  constexpr int kDelimiterTypeCount = 3;
  DelimiterOffset openingStacks[kDelimiterTypeCount][kInvalidDelimitersCapacity];
  int openingDepth[kDelimiterTypeCount] = {0, 0, 0};
  DelimiterDepth delimiterDepth = 0;

  m_invalidOpeningsCount = 0;
  m_invalidClosingsCount = 0;
  m_lineDepthCount = 1;
  m_lineStartDelimiterDepths[0] = 0;

  const char * fullText = editedText();
  const char * lineStart = fullText;
  int currentLine = 1;

  mp_lexer_t * lex = mp_lexer_new_from_str_len(0, fullText, strlen(fullText), 0);
  while (lex->tok_kind != MP_TOKEN_END && lex->tok_kind != MP_TOKEN_FSTRING_RAW) {
    while (currentLine < lex->tok_line && !UTF8Helper::CodePointIs(lineStart, UCodePointNull)) {
      const char * nextLine = UTF8Helper::CodePointSearch(lineStart, '\n');
      if (UTF8Helper::CodePointIs(nextLine, UCodePointNull)) {
        break;
      }
      lineStart = nextLine + 1;
      currentLine++;
      if (m_lineDepthCount < kLineDepthCapacity) {
        m_lineStartDelimiterDepths[m_lineDepthCount++] = delimiterDepth;
      }
    }

    int delimiterType = DelimiterTypeIndex(lex->tok_kind);
    if (delimiterType >= 0) {
      const char * tokenPosition = lineStart + lex->tok_column - 1;
      DelimiterOffset tokenOffset = OffsetForPosition(fullText, tokenPosition);
      if (IsOpeningDelimiter(lex->tok_kind)) {
        if (openingDepth[delimiterType] < kInvalidDelimitersCapacity) {
          openingStacks[delimiterType][openingDepth[delimiterType]] = tokenOffset;
        }
        openingDepth[delimiterType]++;
        delimiterDepth = NextDelimiterDepth(delimiterDepth);
      } else if (IsClosingDelimiter(lex->tok_kind)) {
        if (openingDepth[delimiterType] > 0) {
          openingDepth[delimiterType]--;
          delimiterDepth = PreviousDelimiterDepth(delimiterDepth);
        } else if (tokenOffset != UINT16_MAX && m_invalidClosingsCount < kInvalidDelimitersCapacity) {
          m_invalidClosings[m_invalidClosingsCount++] = tokenOffset;
        }
      }
    }
    mp_lexer_to_next(lex);
  }
  mp_lexer_free(lex);

  while (!UTF8Helper::CodePointIs(lineStart, UCodePointNull)) {
    const char * nextLine = UTF8Helper::CodePointSearch(lineStart, '\n');
    if (UTF8Helper::CodePointIs(nextLine, UCodePointNull)) {
      break;
    }
    lineStart = nextLine + 1;
    if (m_lineDepthCount < kLineDepthCapacity) {
      m_lineStartDelimiterDepths[m_lineDepthCount++] = delimiterDepth;
    }
  }

  for (int delimiterType = 0; delimiterType < kDelimiterTypeCount; delimiterType++) {
    int remainingOpenings = std::min(openingDepth[delimiterType], kInvalidDelimitersCapacity);
    for (int i = 0; i < remainingOpenings && m_invalidOpeningsCount < kInvalidDelimitersCapacity; i++) {
      if (openingStacks[delimiterType][i] != UINT16_MAX) {
        m_invalidOpenings[m_invalidOpeningsCount++] = openingStacks[delimiterType][i];
      }
    }
  }

  /* Recompute per-line delimiter depth with the same validity rules as
   * drawLine: invalid openings/closings do not change depth. This keeps
   * multiline rainbow colors consistent when unmatched delimiters exist on
   * previous lines. */
  delimiterDepth = 0;
  m_lineDepthCount = 1;
  m_lineStartDelimiterDepths[0] = 0;
  lineStart = fullText;
  currentLine = 1;

  lex = mp_lexer_new_from_str_len(0, fullText, strlen(fullText), 0);
  while (lex->tok_kind != MP_TOKEN_END && lex->tok_kind != MP_TOKEN_FSTRING_RAW) {
    while (currentLine < lex->tok_line && !UTF8Helper::CodePointIs(lineStart, UCodePointNull)) {
      const char * nextLine = UTF8Helper::CodePointSearch(lineStart, '\n');
      if (UTF8Helper::CodePointIs(nextLine, UCodePointNull)) {
        break;
      }
      lineStart = nextLine + 1;
      currentLine++;
      if (m_lineDepthCount < kLineDepthCapacity) {
        m_lineStartDelimiterDepths[m_lineDepthCount++] = delimiterDepth;
      }
    }

    int delimiterType = DelimiterTypeIndex(lex->tok_kind);
    if (delimiterType >= 0) {
      const char * tokenPosition = lineStart + lex->tok_column - 1;
      DelimiterOffset tokenOffset = OffsetForPosition(fullText, tokenPosition);
      bool isInvalidOpening = tokenOffset != UINT16_MAX
        && OffsetInList(tokenOffset, m_invalidOpenings, m_invalidOpeningsCount);
      bool isInvalidClosing = tokenOffset != UINT16_MAX
        && OffsetInList(tokenOffset, m_invalidClosings, m_invalidClosingsCount);

      if (IsOpeningDelimiter(lex->tok_kind) && !isInvalidOpening) {
        delimiterDepth = NextDelimiterDepth(delimiterDepth);
      } else if (IsClosingDelimiter(lex->tok_kind) && !isInvalidClosing) {
        delimiterDepth = PreviousDelimiterDepth(delimiterDepth);
      }
    }
    mp_lexer_to_next(lex);
  }
  mp_lexer_free(lex);

  while (!UTF8Helper::CodePointIs(lineStart, UCodePointNull)) {
    const char * nextLine = UTF8Helper::CodePointSearch(lineStart, '\n');
    if (UTF8Helper::CodePointIs(nextLine, UCodePointNull)) {
      break;
    }
    lineStart = nextLine + 1;
    if (m_lineDepthCount < kLineDepthCapacity) {
      m_lineStartDelimiterDepths[m_lineDepthCount++] = delimiterDepth;
    }
  }

  m_delimiterColoringCacheIsValid = true;
}

void PythonTextArea::ContentView::clearRect(KDContext * ctx, KDRect rect) const {
  ctx->fillRect(rect, BackgroundColor);
}

#define LOG_DRAWING 0
#if LOG_DRAWING
#include <stdio.h>
#define LOG_DRAW(...) printf(__VA_ARGS__)
#else
#define LOG_DRAW(...)
#endif

void PythonTextArea::ContentView::drawLine(KDContext * ctx, int line, const char * text, size_t byteLength, int fromColumn, int toColumn, const char * selectionStart, const char * selectionEnd) const {
  LOG_DRAW("Drawing \"%.*s\"\n", byteLength, text);

  assert(m_pythonDelegate->isPythonUser(this));

  /* We're using the MicroPython lexer to do syntax highlighting on a per-line
   * basis. This can work, however the MicroPython lexer won't accept a line
   * starting with a whitespace. So we're discarding leading whitespaces
   * beforehand. */
  const char * firstNonSpace = UTF8Helper::NotCodePointSearch(text, ' ');
  if (firstNonSpace != text) {
    // Color the discarded leading whitespaces
    const char * spacesStart = UTF8Helper::CodePointAtGlyphOffset(text, fromColumn);
    drawStringAt(
        ctx,
        line,
        fromColumn,
        spacesStart,
        std::min(text + byteLength, firstNonSpace) - spacesStart,
        StringColor,
        BackgroundColor,
        selectionStart,
        selectionEnd,
        HighlightColor,
        false);
  }
  if (UTF8Helper::CodePointIs(firstNonSpace, UCodePointNull)) {
    return;
  }

  const char * autocompleteStart = m_autocomplete ? m_cursorLocation : nullptr;

  nlr_buf_t nlr;
  if (nlr_push(&nlr) == 0) {
    int delimiterDepth = delimiterDepthAtLine(line);

    mp_lexer_t * lex = mp_lexer_new_from_str_len(0, firstNonSpace, byteLength - (firstNonSpace - text), 0);
    LOG_DRAW("Pop token %d\n", lex->tok_kind);

    const char * tokenFrom = firstNonSpace;
    size_t tokenLength = 0;
    const char * tokenEnd = firstNonSpace;
    while (lex->tok_kind != MP_TOKEN_NEWLINE && lex->tok_kind != MP_TOKEN_END && lex->tok_kind != MP_TOKEN_FSTRING_RAW) {
      tokenFrom = firstNonSpace + lex->tok_column - 1;
      if (tokenFrom != tokenEnd) {
        // We passed over white spaces, we need to color them
        drawStringAt(
            ctx,
            line,
            UTF8Helper::GlyphOffsetAtCodePoint(text, tokenEnd),
            tokenEnd,
            std::min(text + byteLength, tokenFrom) - tokenEnd,
            StringColor,
            BackgroundColor,
            selectionStart,
            selectionEnd,
            HighlightColor,
            false);
      }
      tokenLength = TokenLength(lex, tokenFrom);
      tokenEnd = tokenFrom + tokenLength;

      // If the token is being autocompleted, use DefaultColor/Font
      KDColor color = (tokenFrom <= autocompleteStart && autocompleteStart < tokenEnd) ? Palette::CodeText : TokenColor(lex->tok_kind);
      bool italic = (tokenFrom <= autocompleteStart && autocompleteStart < tokenEnd) ? false : isItalic(lex->tok_kind);

      // Only apply rainbow/invalid coloring to delimiters when syntax
      // highlighting is enabled. Otherwise delimiters stay as normal text.
      if (GlobalPreferences::sharedGlobalPreferences()->syntaxhighlighting()) {
        if (IsOpeningDelimiter(lex->tok_kind)) {
          bool invalidOpening = isInvalidOpeningDelimiter(tokenFrom);
          if (invalidOpening) {
            color = InvalidParenthesisColor;
          } else {
            color = ParenthesisColorForDepth(delimiterDepth);
            delimiterDepth = NextDelimiterDepth(delimiterDepth);
          }
        } else if (IsClosingDelimiter(lex->tok_kind)) {
          bool invalidClosing = isInvalidClosingDelimiter(tokenFrom);
          if (!invalidClosing) {
            delimiterDepth = PreviousDelimiterDepth(delimiterDepth);
            color = ParenthesisColorForDepth(delimiterDepth);
          } else {
            color = InvalidParenthesisColor;
          }
        }
      }

      LOG_DRAW("Draw \"%.*s\" for token %d\n", tokenLength, tokenFrom, lex->tok_kind);
      drawStringAt(ctx, line,
        UTF8Helper::GlyphOffsetAtCodePoint(text, tokenFrom),
        tokenFrom,
        tokenLength,
        color,
        BackgroundColor,
        selectionStart,
        selectionEnd,
        HighlightColor,
        italic
      );

      mp_lexer_to_next(lex);
      LOG_DRAW("Pop token %d\n", lex->tok_kind);
    }

    tokenFrom += tokenLength;

    KDColor color = CommentColor;
    if (!GlobalPreferences::sharedGlobalPreferences()->syntaxhighlighting()) {
      color = Palette::CodeText;
    }
    // Even if the token is being autocompleted, use CommentColor
    if (tokenFrom < text + byteLength) {
      LOG_DRAW("Draw comment \"%.*s\" from %d\n", byteLength - (tokenFrom - text), firstNonSpace, tokenFrom);
      drawStringAt(ctx, line,
          UTF8Helper::GlyphOffsetAtCodePoint(text, tokenFrom),
          tokenFrom,
          text + byteLength - tokenFrom,
          color,
          BackgroundColor,
          selectionStart,
          selectionEnd,
          HighlightColor,
          true);
    }

    mp_lexer_free(lex);
    nlr_pop();
  }

  // Redraw the autocompleted word in the right color
  if (m_autocomplete && autocompleteStart >= text && autocompleteStart < text + byteLength) {
    assert(m_autocompletionEnd != nullptr && m_autocompletionEnd > autocompleteStart);
    drawStringAt(
        ctx,
        line,
        UTF8Helper::GlyphOffsetAtCodePoint(text, autocompleteStart),
        autocompleteStart,
        std::min(text + byteLength, m_autocompletionEnd) - autocompleteStart,
        AutocompleteColor,
        BackgroundColor,
        nullptr,
        nullptr,
        HighlightColor,
        false);
  }

  /* Lightweight per-line indent guides: draw a vertical bar at each tab
   * stop within the leading whitespace of this line. This is cheap and uses
   * no persistent memory. */
  {
    int leadingGlyphs = UTF8Helper::GlyphOffsetAtCodePoint(text, firstNonSpace);
    if (leadingGlyphs > 0) {
      const int kTabWidth = 2;
      int guideCount = 0;
      if (leadingGlyphs > 0) {
        guideCount = (leadingGlyphs - 1) / kTabWidth;
      }
      if (guideCount > 0) {
        KDSize glyphSize = m_font->glyphSize();
        KDCoordinate y = line * glyphSize.height();
        KDCoordinate barWidth = 1;
        for (int g = 1; g <= guideCount; g++) {
          int column = g * kTabWidth;
          const char * cursorPos = UTF8Helper::CodePointAtGlyphOffset(text, column);
          KDCoordinate x = m_font->stringSizeUntil(text, cursorPos).width();
          KDCoordinate barX = x - barWidth / 2;
          if (barX < 0) barX = 0;
          ctx->fillRect(KDRect(barX, y, barWidth, glyphSize.height()), IndentGuideColor);
        }
      }
    }
  }
}

KDRect PythonTextArea::ContentView::dirtyRectFromPosition(const char * position, bool includeFollowingLines) const {
  /* Mark the whole line as dirty.
   * TextArea has a very conservative approach and only dirties the surroundings
   * of the current character. That works for plain text, but when doing syntax
   * highlighting, you may want to redraw the surroundings as well. For example,
   * if editing "def foo" into "df foo", you'll want to redraw "df". */
  KDRect baseDirtyRect = TextArea::ContentView::dirtyRectFromPosition(position, includeFollowingLines);
  /* Delimiter colors depend on all previous delimiters, potentially across
   * multiple lines. Redraw from the edited line to the bottom to keep colors in
   * sync after insertions/deletions. */
  KDCoordinate dirtyHeight = bounds().height() - baseDirtyRect.y();
  if (dirtyHeight < 0) {
    dirtyHeight = 0;
  }
  return KDRect(
    bounds().x(),
    baseDirtyRect.y(),
    bounds().width(),
    dirtyHeight
  );
}

bool PythonTextArea::handleEvent(Ion::Events::Event event) {
  bool shouldRefreshVisibleArea = false;
  if (m_contentView.isAutocompleting()) {
    // Handle event with autocompletion
    if (event == Ion::Events::Right
        || event == Ion::Events::ShiftRight
        || event == Ion::Events::OK)
    {
      m_contentView.reloadRectFromPosition(m_contentView.cursorLocation(), false);
      acceptAutocompletion(event != Ion::Events::ShiftRight);
      if (event != Ion::Events::ShiftRight) {
        // Do not process the event more
        scrollToCursor();
        return true;
      }
    } else if (event == Ion::Events::Toolbox
        || event == Ion::Events::Var
        || event == Ion::Events::Shift
        || event == Ion::Events::Alpha
        || event == Ion::Events::OnOff)
    {
    } else if(event == Ion::Events::Up
        || event == Ion::Events::Down)
    {
      cycleAutocompletion(event == Ion::Events::Down);
      return true;
    } else {
      removeAutocompletion();
      m_contentView.reloadRectFromPosition(m_contentView.cursorLocation(), false);
      if (event == Ion::Events::Back) {
        // Do not process the event more
        return true;
      }
    }
  }

  if (!selectionIsEmpty() && (event == Ion::Events::Space || event == Ion::Events::ShiftSpace)) {
    constexpr int kIndentWidth = 2;
    constexpr int kMaxSelectedLines = 1024;
    Ion::Events::ShiftAlphaStatus shiftAlphaStatus = Ion::Events::shiftAlphaStatus();
    const bool shiftPressed = Ion::Keyboard::scan().keyDown(Ion::Keyboard::Key::Shift)
      || event == Ion::Events::ShiftSpace
      || Ion::Events::isShiftActive()
      || shiftAlphaStatus == Ion::Events::ShiftAlphaStatus::ShiftAlpha
      || shiftAlphaStatus == Ion::Events::ShiftAlphaStatus::ShiftAlphaLock;

    if (m_contentView.isAutocompleting()) {
      removeAutocompletion();
      m_contentView.reloadRectFromPosition(m_contentView.cursorLocation(), false);
    }

    const char * text = m_contentView.editedText();
    int selectionStartOffset = m_contentView.selectionStart() - text;
    int selectionEndOffset = m_contentView.selectionEnd() - text;
    int cursorOffset = cursorLocation() - text;

    int lineStartOffsets[kMaxSelectedLines];
    int lineCount = SelectedLineStartOffsets(text, selectionStartOffset, selectionEndOffset, lineStartOffsets, kMaxSelectedLines);
    if (lineCount == 0) {
      return true;
    }

    if (!shiftPressed) {
      size_t textLength = m_contentView.getText()->textLength();
      size_t bufferSize = m_contentView.getText()->bufferSize();
      if (textLength + lineCount * kIndentWidth >= bufferSize) {
        return true;
      }

      int addedSoFar = 0;
      for (int i = 0; i < lineCount; i++) {
        const int insertionOffset = lineStartOffsets[i] + addedSoFar;
        const char spaces[] = "  ";
        m_contentView.insertTextAtLocation(spaces, const_cast<char *>(text + insertionOffset), kIndentWidth);
        selectionStartOffset = OffsetAfterInsertion(selectionStartOffset, insertionOffset, kIndentWidth);
        selectionEndOffset = OffsetAfterInsertion(selectionEndOffset, insertionOffset, kIndentWidth);
        cursorOffset = OffsetAfterInsertion(cursorOffset, insertionOffset, kIndentWidth);
        addedSoFar += kIndentWidth;
      }
    } else {
      int removedSoFar = 0;
      for (int i = 0; i < lineCount; i++) {
        const int lineOffset = lineStartOffsets[i] - removedSoFar;
        const char * lineStart = text + lineOffset;
        int removableSpaces = 0;
        while (removableSpaces < kIndentWidth && lineStart[removableSpaces] == ' ') {
          removableSpaces++;
        }
        if (removableSpaces == 0) {
          continue;
        }

        m_contentView.removeText(lineStart, lineStart + removableSpaces);
        selectionStartOffset = OffsetAfterDeletion(selectionStartOffset, lineOffset, removableSpaces);
        selectionEndOffset = OffsetAfterDeletion(selectionEndOffset, lineOffset, removableSpaces);
        cursorOffset = OffsetAfterDeletion(cursorOffset, lineOffset, removableSpaces);
        removedSoFar += removableSpaces;
      }
    }

    const char * updatedText = m_contentView.editedText();
    setCursorLocation(updatedText + cursorOffset);
    m_contentView.resetSelection();
    m_contentView.addSelection(updatedText + selectionStartOffset, updatedText + selectionEndOffset);
    m_contentView.invalidateDelimiterColoringCache();
    m_contentView.reloadRectFromPosition(updatedText, true);
    scrollToCursor();
    return true;
  }

  if (event == Ion::Events::Backspace && !m_contentView.isAutocompleting() && selectionIsEmpty()) {
    const char * text = m_contentView.editedText();
    const char * cursor = cursorLocation();
    if (cursor > text) {
      const char openingDelimiter = *(cursor - 1);
      const char matchingClosingDelimiter = MatchingClosingDelimiterChar(openingDelimiter);
      if (GlobalPreferences::sharedGlobalPreferences()->autoCloseParentheses() && matchingClosingDelimiter != 0 && *cursor == matchingClosingDelimiter) {
        // Compare deleting only the opening delimiter vs deleting the pair
        // (opening + closing). Choose the option that minimizes invalid
        // delimiters in a local window.
        const char * openingPosition = cursor - 1;
        int deltaSingle = m_contentView.estimateInvalidDeltaForDeletion(openingPosition, 1, 512);
        int deltaPair = m_contentView.estimateInvalidDeltaForDeletion(openingPosition, 2, 512);
        if (deltaPair < deltaSingle) {
          m_contentView.removeText(openingPosition, cursor + 1);
        } else {
          m_contentView.removeText(openingPosition, cursor);
        }
        setCursorLocation(openingPosition);
        m_contentView.invalidateDelimiterColoringCache();
        addAutocompletion();
        m_contentView.reloadRectFromPosition(m_contentView.editedText(), true);
        scrollToCursor();
        return true;
      }
    }
  }

  bool result = TextArea::handleEvent(event);
  if (event == Ion::Events::Backspace && !m_contentView.isAutocompleting() && selectionIsEmpty()) {
    m_contentView.invalidateDelimiterColoringCache();
    /* We want to add autocompletion when we are editing a word (after adding or
     * deleting text). So if nothing is selected, we add the autocompletion if
     * the event is backspace, as autocompletion has already been added if the
     * event added text, in handleEventWithText. */
    addAutocompletion();
    shouldRefreshVisibleArea = true;
  }
  if (shouldRefreshVisibleArea) {
    m_contentView.reloadRectFromPosition(m_contentView.editedText(), true);
  }
  return result;
}

bool PythonTextArea::handleEventWithText(const char * text, bool indentation, bool forceCursorRightOfText, bool shouldRemoveLastCharacter) {
  if (*text == 0) {
    return false;
  }
  bool singleDelimiterInput = text[1] == 0 && (IsOpeningDelimiterChar(*text) || IsClosingDelimiterChar(*text));

  if (singleDelimiterInput && selectionIsEmpty()) {
    if (m_contentView.isAutocompleting()) {
      removeAutocompletion();
    }

    const char currentDelimiter = *text;
    if (IsClosingDelimiterChar(currentDelimiter) && GlobalPreferences::sharedGlobalPreferences()->autoCloseParentheses() && *cursorLocation() == currentDelimiter) {
      // Compare the two options: inserting the typed closing delimiter
      // here vs skipping over the existing closing delimiter. Choose the
      // option that yields fewer invalid delimiters locally.
      int delta = m_contentView.estimateInvalidDeltaForInsertion(cursorLocation(), &currentDelimiter, 1, 512);
      if (delta < 0 && m_contentView.isAbleToInsertTextAt(1, cursorLocation(), false)) {
        // Inserting reduces invalids: perform the insertion via base
        // TextArea implementation (user-initiated insertion).
        TextArea::handleEventWithText(text, indentation, forceCursorRightOfText, shouldRemoveLastCharacter);
        m_contentView.invalidateDelimiterColoringCache();
        m_contentView.reloadRectFromPosition(m_contentView.editedText(), true);
        return true;
      }
      // Otherwise, prefer skipping over the existing closing delimiter.
      setCursorLocation(cursorLocation() + 1);
      scrollToCursor();
      return true;
    }

    if (IsOpeningDelimiterChar(currentDelimiter)) {
      bool insertedOpening = TextArea::handleEventWithText(text, indentation, forceCursorRightOfText, shouldRemoveLastCharacter);
      if (!insertedOpening) {
        return false;
      }

      char closingChar = MatchingClosingDelimiterChar(currentDelimiter);
      char closingDelimiterText[2] = {closingChar, 0};
      const char * middlePosition = cursorLocation();
      if (closingChar != 0 && GlobalPreferences::sharedGlobalPreferences()->autoCloseParentheses()) {
        // Estimate the change in invalid delimiters if we insert the
        // closing char here. Insert if it does not increase invalids
        // (delta <= 0). This allows nested insertions like "(((" ->
        // "((()))" when appropriate.
        int delta = m_contentView.estimateInvalidDeltaForInsertion(middlePosition, &closingChar, 1, 512);
        if (delta <= 0 && m_contentView.isAbleToInsertTextAt(1, middlePosition, false)) {
          m_contentView.insertTextAtLocation(closingDelimiterText, const_cast<char *>(middlePosition), 1);
          // After insertion, keep the cursor between the pair
          setCursorLocation(middlePosition);
        }
      }

      m_contentView.invalidateDelimiterColoringCache();
      m_contentView.reloadRectFromPosition(m_contentView.editedText(), true);
      addAutocompletion();
      return true;
    }
  }

  bool shouldRefreshVisibleArea = TextContainsDelimiter(text);
  if (m_contentView.isAutocompleting()) {
    removeAutocompletion();
  }
  bool result = TextArea::handleEventWithText(text, indentation, forceCursorRightOfText, shouldRemoveLastCharacter);
  m_contentView.invalidateDelimiterColoringCache();
  if (shouldRefreshVisibleArea) {
    m_contentView.reloadRectFromPosition(m_contentView.editedText(), true);
  }
  addAutocompletion();
  return result;
}

void PythonTextArea::removeAutocompletion() {
  assert(m_contentView.isAutocompleting());
  removeAutocompletionText();
  m_contentView.setAutocompleting(false);
}

void PythonTextArea::removeAutocompletionText() {
  assert(m_contentView.isAutocompleting());
  assert(m_contentView.autocompletionEnd() != nullptr);
  const char * autocompleteStart = m_contentView.cursorLocation();
  const char * autocompleteEnd = m_contentView.autocompletionEnd();
  assert(autocompleteEnd != nullptr && autocompleteEnd > autocompleteStart);
  m_contentView.removeText(autocompleteStart, autocompleteEnd);
}

void PythonTextArea::addAutocompletion() {
  assert(!m_contentView.isAutocompleting());
  const char * autocompletionTokenBeginning = nullptr;
  const char * autocompletionLocation = const_cast<char *>(cursorLocation());
  m_autocompletionResultIndex = 0;
  if (autocompletionType(autocompletionLocation, &autocompletionTokenBeginning) != AutocompletionType::EndOfIdentifier) {
    // The cursor is not at the end of an identifier.
    return;
  }

  // First load variables and functions that complete the textToAutocomplete
  const int scriptIndex = m_contentView.pythonDelegate()->menuController()->editedScriptIndex();
  m_contentView.pythonDelegate()->variableBoxController()->loadFunctionsAndVariables(scriptIndex, autocompletionTokenBeginning, autocompletionLocation - autocompletionTokenBeginning);

  addAutocompletionTextAtIndex(0);
}

bool PythonTextArea::addAutocompletionTextAtIndex(int nextIndex, int * currentIndexToUpdate) {
  // If Autocomplete disable, skip this step
  if(!GlobalPreferences::sharedGlobalPreferences()->autocomplete()) {
    return false;
  }

  // The variable box should be loaded at this point
  const char * autocompletionTokenBeginning = nullptr;
  const char * autocompletionLocation = const_cast<char *>(cursorLocation());
  AutocompletionType type = autocompletionType(autocompletionLocation, &autocompletionTokenBeginning); // Done to get autocompletionTokenBeginning
  assert(type == AutocompletionType::EndOfIdentifier);
  (void)type; // Silence warnings
  VariableBoxController * varBox = m_contentView.pythonDelegate()->variableBoxController();
  int textToInsertLength = 0;
  bool addParentheses = false;
  const char * textToInsert = varBox->autocompletionAlternativeAtIndex(autocompletionLocation - autocompletionTokenBeginning, &textToInsertLength, &addParentheses, nextIndex, currentIndexToUpdate);

  if (textToInsert == nullptr) {
    return false;
  }

  if (textToInsertLength > 0) {
    // Try to insert the text (this might fail if the buffer is full)
    if (!m_contentView.isAbleToInsertTextAt(textToInsertLength, autocompletionLocation, false)) {
      return false;
    }
    m_contentView.insertTextAtLocation(textToInsert, const_cast<char *>(autocompletionLocation), textToInsertLength);
    autocompletionLocation += textToInsertLength;
    m_contentView.setAutocompleting(true);
    m_contentView.setAutocompletionEnd(autocompletionLocation);
  }

  // Try to insert the parentheses if needed
  const char * parentheses = ScriptNodeCell::k_parentheses;
  constexpr int parenthesesLength = 2;
  assert(strlen(parentheses) == parenthesesLength);
  /* If couldInsertText is false, we should not try to add the parentheses as
   * there was already not enough space to add the autocompletion. */
  if (addParentheses) {
    // Estimate the local effect of inserting `()` here and only do it if it
    // does not increase invalid delimiters (delta <= 0). This permits
    // autocompletion to add parentheses when it helps nesting/matching.
    int delta = m_contentView.estimateInvalidDeltaForInsertion(autocompletionLocation, parentheses, parenthesesLength, 512);
    if (delta <= 0 && m_contentView.isAbleToInsertTextAt(parenthesesLength, autocompletionLocation, false)) {
      m_contentView.insertTextAtLocation(parentheses, const_cast<char *>(autocompletionLocation), parenthesesLength);
      m_contentView.setAutocompleting(true);
      m_contentView.setAutocompletionEnd(autocompletionLocation + parenthesesLength);
      return true;
    }
  }
  return (textToInsertLength > 0);
}

void PythonTextArea::cycleAutocompletion(bool downwards) {
  assert(m_contentView.isAutocompleting());
  removeAutocompletionText();
  addAutocompletionTextAtIndex(m_autocompletionResultIndex + (downwards ? 1 : -1), &m_autocompletionResultIndex);
}

void PythonTextArea::acceptAutocompletion(bool moveCursorToEndOfAutocompletion) {
  assert(m_contentView.isAutocompleting());

  // Save the cursor location
  const char * previousCursorLocation = cursorLocation();

  removeAutocompletion();

  m_contentView.pythonDelegate()->variableBoxController()->setSender(this);
  m_contentView.pythonDelegate()->variableBoxController()->insertAutocompletionResultAtIndex(m_autocompletionResultIndex);

  // insertAutocompletionResultAtIndex already added the autocompletion

  // If we did not want to move the cursor, restore its position.
  if (!moveCursorToEndOfAutocompletion) {
    setCursorLocation(previousCursorLocation);
  }
}

} // namespace Code

#undef CommentColor
#undef NumberColor
#undef KeywordColor
#undef OperatorColor
#undef StringColor
#undef BackgroundColor
#undef HighlightColor
#undef InvalidParenthesisColor
#undef IndentGuideColor
