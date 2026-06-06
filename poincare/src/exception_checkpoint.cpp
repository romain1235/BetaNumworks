#include <poincare/exception_checkpoint.h>

namespace Poincare {

ExceptionCheckpoint * ExceptionCheckpoint::s_topmostExceptionCheckpoint;

ExceptionCheckpoint::ExceptionCheckpoint(bool poolInitialized) :
  m_parent(s_topmostExceptionCheckpoint)
{
  if (poolInitialized) {
    m_endOfPoolBeforeCheckpoint = TreePool::sharedPool()->last();
  } else {
    m_endOfPoolBeforeCheckpoint = nullptr;
  }
  s_topmostExceptionCheckpoint = this;
}

/*
int ExceptionCheckpoint::run() {
  m_endOfPoolBeforeCheckpoint = TreePool::sharedPool()->last();
  m_parent = s_topmostExceptionCheckpoint;
  s_topmostExceptionCheckpoint = this;
  return setjmp(m_jumpBuffer) == 0;
}
*/

void ExceptionCheckpoint::rollback() {
  // If the pool isn't initialized, don't attempt to restore the pool
  if (m_endOfPoolBeforeCheckpoint != nullptr) {
    Poincare::TreePool::sharedPool()->freePoolFromNode(m_endOfPoolBeforeCheckpoint);
  }
  longjmp(m_jumpBuffer, 1);
}

}
