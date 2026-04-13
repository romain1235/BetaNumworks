#include <poincare/code_point_layout.h>
#include <poincare/layout_helper.h>
#include <poincare/serialization_helper.h>

namespace Poincare {

// LayoutNode
void CodePointLayoutNode::moveCursorLeft(LayoutCursor * cursor, bool * shouldRecomputeLayout, bool forSelection) {
  if (cursor->position() == LayoutCursor::Position::Right) {
    cursor->setPosition(LayoutCursor::Position::Left);
    return;
  }
  LayoutNode * parentNode = parent();
  if (parentNode != nullptr) {
    parentNode->moveCursorLeft(cursor, shouldRecomputeLayout);
  }
}

void CodePointLayoutNode::moveCursorRight(LayoutCursor * cursor, bool * shouldRecomputeLayout, bool forSelection) {
  if (cursor->position() == LayoutCursor::Position::Left) {
    cursor->setPosition(LayoutCursor::Position::Right);
    return;
  }
  LayoutNode * parentNode = parent();
  if (parentNode != nullptr) {
    parentNode->moveCursorRight(cursor, shouldRecomputeLayout);
  }
}

int CodePointLayoutNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return SerializationHelper::CodePoint(buffer, bufferSize, m_codePoint);
}

bool CodePointLayoutNode::isCollapsable(int * numberOfOpenParenthesis, bool goingLeft) const {
  if (*numberOfOpenParenthesis <= 0) {
    if (m_codePoint == '+'
        || m_codePoint == UCodePointRightwardsArrow
        || m_codePoint == '='
        || m_codePoint == ',')
    {
      return false;
    }
    if (m_codePoint == '-') {
      /* If the expression is like 3ᴇ-200, we want '-' to be collapsable.
       * Otherwise, '-' is not collapsable. */
      Layout thisRef = CodePointLayout(this);
      Layout parent = thisRef.parent();
      if (!parent.isUninitialized()) {
        int indexOfThis = parent.indexOfChild(thisRef);
        if (indexOfThis > 0) {
          Layout leftBrother = parent.childAtIndex(indexOfThis-1);
          if (leftBrother.type() == Type::CodePointLayout
              && static_cast<CodePointLayout&>(leftBrother).codePoint() == UCodePointLatinLetterSmallCapitalE)
          {
            return true;
          }
        }
      }
      return false;
    }
    if (isMultiplicationCodePoint()) {
      /* We want '*' to be collapsable only if the following brother is not a
       * fraction, so that the user can write intuitively "1/2 * 3/4". */
      Layout thisRef = CodePointLayout(this);
      Layout parent = thisRef.parent();
      if (!parent.isUninitialized()) {
        int indexOfThis = parent.indexOfChild(thisRef);
        Layout brother;
        if (indexOfThis > 0 && goingLeft) {
          brother = parent.childAtIndex(indexOfThis-1);
        } else if (indexOfThis < parent.numberOfChildren() - 1 && !goingLeft) {
          brother = parent.childAtIndex(indexOfThis+1);
        }
        if (!brother.isUninitialized() && brother.type() == LayoutNode::Type::FractionLayout) {
          return false;
        }
      }
    }
  }
  return true;
}

bool CodePointLayoutNode::canBeOmittedMultiplicationLeftFactor() const {
  if (isMultiplicationCodePoint()) {
    return false;
  }
  return LayoutNode::canBeOmittedMultiplicationLeftFactor();
}

bool CodePointLayoutNode::canBeOmittedMultiplicationRightFactor() const {
  if (m_codePoint == '!' || isMultiplicationCodePoint()) {
    return false;
  }
  return LayoutNode::canBeOmittedMultiplicationRightFactor();
}

// Sizing and positioning
KDSize CodePointLayoutNode::computeSize() {
  KDSize base = m_font->glyphSize();
  // If grouping is disabled, return base size
  KDCoordinate groupingSpacing = ThousandsGroupingSpacing();
  if (groupingSpacing == 0) {
    return base;
  }

  // Only consider grouping for ASCII digits
  if (m_codePoint < '0' || m_codePoint > '9') {
    return base;
  }

  // Use raw parent node access (safer during layout computations)
  LayoutNode * parentNode = parent();
  if (parentNode == nullptr || parentNode->type() != LayoutNode::Type::HorizontalLayout) {
    return base;
  }

  int idx = parentNode->indexOfChild(this);
  // find contiguous run of CodePointLayout children (work with raw nodes)
  int left = idx;
  while (left > 0 && parentNode->childAtIndex(left-1)->type() == LayoutNode::Type::CodePointLayout) {
    left--;
  }
  int right = idx;
  int nChildren = parentNode->numberOfChildren();
  while (right + 1 < nChildren && parentNode->childAtIndex(right+1)->type() == LayoutNode::Type::CodePointLayout) {
    right++;
  }

  // Find contiguous digit-only sub-run that contains idx
  int leftDigits = idx;
  while (leftDigits > left) {
    LayoutNode * prev = parentNode->childAtIndex(leftDigits-1);
    if (prev->type() != LayoutNode::Type::CodePointLayout) break;
    CodePoint c = static_cast<CodePointLayoutNode *>(prev)->codePoint();
    if (c < '0' || c > '9') break;
    leftDigits--;
  }
  int rightDigits = idx;
  while (rightDigits + 1 <= right) {
    LayoutNode * next = parentNode->childAtIndex(rightDigits+1);
    if (next->type() != LayoutNode::Type::CodePointLayout) break;
    CodePoint c = static_cast<CodePointLayoutNode *>(next)->codePoint();
    if (c < '0' || c > '9') break;
    rightDigits++;
  }

  // If there are no other digits besides current, nothing to group
  if (leftDigits == rightDigits) {
    return base;
  }
  // If the digit run is immediately preceded by a decimal point, it's a
  // fractional part (e.g. ".1234") — do not apply thousands grouping.
  if (leftDigits > 0) {
    LayoutNode * before = parentNode->childAtIndex(leftDigits - 1);
    if (before->type() == LayoutNode::Type::CodePointLayout) {
      CodePoint bc = static_cast<CodePointLayoutNode *>(before)->codePoint();
      if (bc == '.') {
        return base;
      }
    }
  }

  // Detect hex literal like 0xF4240: scan leftwards for a contiguous run of
  // hex digits (0-9, a-f, A-F) that contains the current digit. If that run
  // is preceded by "0x" or "0X", do not apply thousands grouping.
  int leftHex = idx;
  while (leftHex > left) {
    LayoutNode * prev = parentNode->childAtIndex(leftHex - 1);
    if (prev->type() != LayoutNode::Type::CodePointLayout) break;
    CodePoint c = static_cast<CodePointLayoutNode *>(prev)->codePoint();
    bool isHexDigit = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    if (!isHexDigit) break;
    leftHex--;
  }
  if (leftHex >= 2) {
    LayoutNode * p1 = parentNode->childAtIndex(leftHex - 1);
    LayoutNode * p2 = parentNode->childAtIndex(leftHex - 2);
    if (p1->type() == LayoutNode::Type::CodePointLayout && p2->type() == LayoutNode::Type::CodePointLayout) {
      CodePoint c1 = static_cast<CodePointLayoutNode *>(p1)->codePoint();
      CodePoint c2 = static_cast<CodePointLayoutNode *>(p2)->codePoint();
      if (c2 == '0' && (c1 == 'x' || c1 == 'X')) {
        return base;
      }
    }
  }

  // Detect binary literal like 0b1010: scan leftwards for a contiguous run of
  // binary digits (0-1) that contains the current digit. If that run is
  // preceded by "0b" or "0B", do not apply thousands grouping.
  int leftBin = idx;
  while (leftBin > left) {
    LayoutNode * prev = parentNode->childAtIndex(leftBin - 1);
    if (prev->type() != LayoutNode::Type::CodePointLayout) break;
    CodePoint c = static_cast<CodePointLayoutNode *>(prev)->codePoint();
    bool isBinDigit = (c == '0' || c == '1');
    if (!isBinDigit) break;
    leftBin--;
  }
  if (leftBin >= 2) {
    LayoutNode * p1 = parentNode->childAtIndex(leftBin - 1);
    LayoutNode * p2 = parentNode->childAtIndex(leftBin - 2);
    if (p1->type() == LayoutNode::Type::CodePointLayout && p2->type() == LayoutNode::Type::CodePointLayout) {
      CodePoint c1 = static_cast<CodePointLayoutNode *>(p1)->codePoint();
      CodePoint c2 = static_cast<CodePointLayoutNode *>(p2)->codePoint();
      if (c2 == '0' && (c1 == 'b' || c1 == 'B')) {
        return base;
      }
    }
  }

  // integer part is the digit-only run before any decimal point or exponent
  int integerPartRight = rightDigits;
  // Check for a decimal point immediately after the digit run
  if (integerPartRight + 1 <= right) {
    LayoutNode * after = parentNode->childAtIndex(integerPartRight + 1);
    if (after->type() == LayoutNode::Type::CodePointLayout) {
      CodePoint ac = static_cast<CodePointLayoutNode *>(after)->codePoint();
      if (ac == '.' ) {
        // there's a decimal point, keep integerPartRight as is
      }
    }
  }

  int posFromRight = integerPartRight - idx;
  if (posFromRight > 0 && posFromRight % 3 == 0) {
    return KDSize(base.width() + groupingSpacing, base.height());
  }
  return base;
}

KDCoordinate CodePointLayoutNode::computeBaseline() {
  return m_font->glyphSize().height()/2;
}

void CodePointLayoutNode::render(KDContext * ctx, KDPoint p, KDColor expressionColor, KDColor backgroundColor, Layout * selectionStart, Layout * selectionEnd, KDColor selectionColor) {
  constexpr int bufferSize = sizeof(CodePoint)/sizeof(char) + 1; // Null-terminating char
  char buffer[bufferSize];
  SerializationHelper::CodePoint(buffer, bufferSize, m_codePoint);
  ctx->drawString(buffer, p, m_font, expressionColor, backgroundColor);
}

bool CodePointLayoutNode::isMultiplicationCodePoint() const {
  return m_codePoint == '*'
    || m_codePoint == UCodePointMultiplicationSign
    || m_codePoint == UCodePointMiddleDot;
}

bool CodePointLayoutNode::protectedIsIdenticalTo(Layout l) {
  assert(l.type() == Type::CodePointLayout);
  CodePointLayout & cpl = static_cast<CodePointLayout &>(l);
  return codePoint() == cpl.codePoint() && font() == cpl.font();
}

CodePointLayout CodePointLayout::Builder(CodePoint c, const KDFont * font) {
  void * bufferNode = TreePool::sharedPool()->alloc(sizeof(CodePointLayoutNode));
  CodePointLayoutNode * node = new (bufferNode) CodePointLayoutNode(c, font);
  TreeHandle h = TreeHandle::BuildWithGhostChildren(node);
  return static_cast<CodePointLayout &>(h);
}

// Thousands grouping spacing implementation (default enabled: 3px)
static KDCoordinate s_thousandsGroupingSpacing = 3;

void SetThousandsGroupingSpacing(KDCoordinate spacing) {
  s_thousandsGroupingSpacing = spacing;
}

KDCoordinate ThousandsGroupingSpacing() {
  return s_thousandsGroupingSpacing;
}

}
