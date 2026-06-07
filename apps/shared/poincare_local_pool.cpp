#include "poincare_local_pool.h"
#include "poincare/tree_pool.h"

using namespace Poincare;

namespace Shared {

PoincareLocalPool::PoincareLocalPool() :
  m_tree_pool()
{
  Poincare::TreePool::RegisterPool(&m_tree_pool);
  Poincare::TreePool::sharedPool()->reset();
}

void PoincareLocalPool::deinit() {
  Poincare::TreePool::UnregisterPool();
}

}
