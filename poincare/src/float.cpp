#include <poincare/float.h>
#include <poincare/code_point_layout.h>
#include <poincare/horizontal_layout.h>
#include <poincare/layout_helper.h>
#include <poincare/vertical_offset_layout.h>
#include <ion/unicode/utf8_decoder.h>

namespace Poincare {

namespace {

Layout ScientificNotationTextToLayout(const char * buffer, int numberOfChars) {
  assert(numberOfChars > 0);

  const char * exponentMarker = nullptr;
  for (int i = 0; i <= numberOfChars - 4; i++) {
    if (buffer[i] == '*' && buffer[i + 1] == '1' && buffer[i + 2] == '0' && buffer[i + 3] == '^') {
      exponentMarker = buffer + i;
      break;
    }
  }
  int mantissaLength = -1;
  int exponentLength = -1;
  const char * exponentText = nullptr;

  if (exponentMarker != nullptr && exponentMarker < buffer + numberOfChars) {
    mantissaLength = exponentMarker - buffer;
    exponentText = exponentMarker + 4;
    exponentLength = numberOfChars - (exponentText - buffer);
  } else {
    UTF8Decoder decoder(buffer);
    const char * marker = buffer;
    const char * currentPointer = buffer;
    CodePoint codePoint = decoder.nextCodePoint();
    const char * nextPointer = decoder.stringPosition();
    int processedLength = 0;
    while (codePoint != UCodePointNull && processedLength < numberOfChars) {
      if (codePoint == UCodePointLatinLetterSmallCapitalE) {
        marker = currentPointer;
        break;
      }
      processedLength += nextPointer - currentPointer;
      currentPointer = nextPointer;
      codePoint = decoder.nextCodePoint();
      nextPointer = decoder.stringPosition();
    }
    if (marker != buffer) {
      mantissaLength = marker - buffer;
      exponentText = marker + UTF8Decoder::CharSizeOfCodePoint(UCodePointLatinLetterSmallCapitalE);
      exponentLength = numberOfChars - (exponentText - buffer);
    }
  }

  if (mantissaLength <= 0 || exponentLength <= 0) {
    return LayoutHelper::String(buffer, numberOfChars);
  }

  HorizontalLayout result = HorizontalLayout::Builder();
  result.addOrMergeChildAtIndex(LayoutHelper::String(buffer, mantissaLength), result.numberOfChildren(), true);
  result.addChildAtIndex(CodePointLayout::Builder(UCodePointMultiplicationSign), result.numberOfChildren(), result.numberOfChildren(), nullptr);
  result.addChildAtIndex(CodePointLayout::Builder('1'), result.numberOfChildren(), result.numberOfChildren(), nullptr);
  result.addChildAtIndex(CodePointLayout::Builder('0'), result.numberOfChildren(), result.numberOfChildren(), nullptr);
  result.addChildAtIndex(
      VerticalOffsetLayout::Builder(LayoutHelper::String(exponentText, exponentLength), VerticalOffsetLayoutNode::Position::Superscript),
      result.numberOfChildren(),
      result.numberOfChildren(),
      nullptr);
  return std::move(result);
}

}

template<typename T>
Expression FloatNode<T>::setSign(Sign s, ReductionContext reductionContext) {
  assert(s == ExpressionNode::Sign::Positive || s == ExpressionNode::Sign::Negative);
  Sign currentSign = m_value < 0 ? Sign::Negative : Sign::Positive;
  Expression thisExpr = Number(this);
  Expression result = Float<T>::Builder(s == currentSign ? m_value : -m_value);
  thisExpr.replaceWithInPlace(result);
  return result;
}

template<typename T>
int FloatNode<T>::simplificationOrderSameType(const ExpressionNode * e, bool ascending, bool canBeInterrupted, bool ignoreParentheses) const {
  if (!ascending) {
    return e->simplificationOrderSameType(this, true, canBeInterrupted, ignoreParentheses);
  }
  assert((e->type() == ExpressionNode::Type::Float && sizeof(T) == sizeof(float)) || (e->type() == ExpressionNode::Type::Double && sizeof(T) == sizeof(double)));
  const FloatNode<T> * other = static_cast<const FloatNode<T> *>(e);
  if (value() < other->value()) {
    return -1;
  }
  if (value() > other->value()) {
    return 1;
  }
  return 0;
}

template<typename T>
int FloatNode<T>::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return PrintFloat::ConvertFloatToText(m_value, buffer, bufferSize, PrintFloat::k_maxFloatGlyphLength, numberOfSignificantDigits, floatDisplayMode).CharLength;
}

template<typename T>
Layout FloatNode<T>::createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  char buffer[PrintFloat::k_maxFloatCharSize];
  int numberOfChars = serialize(buffer, PrintFloat::k_maxFloatCharSize, floatDisplayMode, numberOfSignificantDigits);
  return ScientificNotationTextToLayout(buffer, numberOfChars);
}

template<typename T>
Float<T> Float<T>::Builder(T value) {
  void * bufferNode = TreePool::sharedPool()->alloc(sizeof(FloatNode<T>));
  FloatNode<T> * node = new (bufferNode) FloatNode<T>(value);
  TreeHandle h = TreeHandle::BuildWithGhostChildren(node);
  return static_cast<Float &>(h);
}

template class FloatNode<float>;
template class FloatNode<double>;

template Float<float> Float<float>::Builder(float value);
template Float<double> Float<double>::Builder(double value);

}
