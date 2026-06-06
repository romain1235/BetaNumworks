#ifndef SHARED_POINCARE_LOCAL_POOL_H
#define SHARED_POINCARE_LOCAL_POOL_H


#include <poincare/tree_pool.h>

namespace Shared {

class PoincareLocalPool  {
public:
  PoincareLocalPool();
  void deinit();
private:
  Poincare::TreePool m_tree_pool;
};

}

#endif
