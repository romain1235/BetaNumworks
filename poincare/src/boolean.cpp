#include <poincare/boolean.h>
#include <poincare/complex.h>
#include <poincare/layout_helper.h>
#include <poincare/tree_pool.h>
#include <algorithm>
#include <string.h>

namespace Poincare {

Expression BooleanNode::setSign(Sign s, ReductionContext reductionContext) {
  return Boolean(static_cast<const BooleanNode *>(this));
}

Boolean Boolean::Builder(bool value) {
  TreePool * pool = TreePool::sharedPool();
  BooleanNode * node = new (pool->alloc(sizeof(BooleanNode))) BooleanNode(value);
  TreeHandle h = TreeHandle::BuildWithGhostChildren(node);
  return static_cast<Boolean &>(h);
}

Layout BooleanNode::createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return LayoutHelper::String(m_value ? Boolean::TrueName() : Boolean::FalseName(), m_value ? 4 : 5);
}

int BooleanNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  const char * name = m_value ? Boolean::TrueName() : Boolean::FalseName();
  return std::min<int>(strlcpy(buffer, name, bufferSize), bufferSize - 1);
}

template<typename T>
Evaluation<T> BooleanNode::templatedApproximate() const {
  return Complex<T>::Builder(m_value ? (T)1.0 : (T)0.0);
}

template Evaluation<float> BooleanNode::templatedApproximate() const;
template Evaluation<double> BooleanNode::templatedApproximate() const;

}
