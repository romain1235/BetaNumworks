#include <quiz.h>
#include "execution_environment.h"

QUIZ_CASE(python_ulab) {
  TestExecutionEnvironment env = init_environnement();
  // Try to import ulab module and submodules
  assert_command_execution_succeeds(env, "import ulab");
  assert_command_execution_succeeds(env, "import ulab as ul");
  assert_command_execution_succeeds(env, "from ulab import *");
  assert_command_execution_succeeds(env, "from ulab import numpy");
  assert_command_execution_succeeds(env, "from ulab import numpy as np");
  assert_command_execution_succeeds(env, "from ulab import scipy");
  assert_command_execution_succeeds(env, "from ulab import scipy as sp");
  assert_command_execution_succeeds(env, "from ulab import scipy as spy");
  // NumPy tests
  assert_command_execution_succeeds(env, "np.array([1, 2, 3])");
  // Store an array in a variable and use it
  assert_command_execution_succeeds(env, "a = np.array([1, 2, 3])");
  assert_command_execution_succeeds(env, "a[0]");
  assert_command_execution_succeeds(env, "a[1]");
  assert_command_execution_succeeds(env, "a[2]");
  assert_command_execution_fails(env, "a[3]");
  // Test np.all
  assert_command_execution_succeeds(env, "np.all([1, 2, 3])");
  // SciPy tests
  // Test ulab.scipy.linalg using spy prefix
  assert_command_execution_succeeds(env, "spy.linalg.solve_triangular(np.array([[1, 2], [3, 4]]), np.array([5, 6]))");
  assert_command_execution_fails(env, "spy.linalg.solve_triangular([[1, 2], [3, 4]], [1, 2, 3])");
  // Test ulab.scipy.optimize using spy prefix
  assert_command_execution_succeeds(env, "spy.optimize.fmin(lambda x: x**2, 1)");
  assert_command_execution_fails(env, "spy.optimize.fmin(lambda x: x**2, 1, maxiter=0)");
  assert_command_execution_succeeds(env, "spy.optimize.fmin(lambda x: x**2, 1, maxiter=1)");
  assert_command_execution_fails(env, "spy.optimize.bisect(lambda x: x**2, 1, 2, maxiter=0)");
  assert_command_execution_succeeds(env, "spy.optimize.newton(lambda x: x**2, 1)");
  assert_command_execution_fails(env, "spy.optimize.newton(lambda x: x**2, 1, maxiter=0)");
  assert_command_execution_succeeds(env, "spy.optimize.newton(lambda x: x**2, 1, maxiter=1)");
  // Test ulab.scipy.signal using spy prefix
  // TODO: Find a way to test this, maybe in a future ulab release ?
  // assert_command_execution_succeeds(env, "spy.signal.sosfilt(np.array([1, 2, 3]), np.array([7, 8, 9]))");
  assert_command_execution_fails(env, "spy.signal.spectrogram(np.array([1, 2, 3]), np.array([7, 8, 9]))");
  // Test ulab.scipy.special using spy prefix
  assert_command_execution_succeeds(env, "spy.special.erf(1)");
  assert_command_execution_fails(env, "spy.special.erf(1, 2)");
  assert_command_execution_succeeds(env, "spy.special.erfc(1)");
  assert_command_execution_fails(env, "spy.special.erfc(1, 2)");
  assert_command_execution_succeeds(env, "spy.special.gamma(1)");
  assert_command_execution_fails(env, "spy.special.gamma(1, 2)");
  assert_command_execution_succeeds(env, "spy.special.gammaln(1)");
  assert_command_execution_fails(env, "spy.special.gammaln(1, 2)");


  // AI generated tests for more coverage: (edited by hand for correctness)
  assert_command_execution_succeeds(env, "a = np.array([[1, 2], [3, 4]])");
  assert_command_execution_succeeds(env, "a.ndim");
  assert_command_execution_succeeds(env, "np.sum(a, keepdims=True)");
  assert_command_execution_succeeds(env, "np.take(np.array([1,2,3]), [0,2])");
  assert_command_execution_fails(env, "np.take(np.array([1,2,3]), [5])");
  assert_command_execution_succeeds(env, "np.bincount(np.array([0,1,1,2], dtype=np.uint16), minlength=4)");
  assert_command_execution_fails(env, "np.bincount(np.array([0,-1,1]))");
  assert_command_execution_succeeds(env, "np.nonzero(np.array([0,1,0,2]))");
  assert_command_execution_succeeds(env, "np.meshgrid(np.array([1,2]), np.array([3,4]))");
  assert_command_execution_succeeds(env, "np.sinc(0)");
  assert_command_execution_succeeds(env, "np.sinc(np.array([0,1,2]))");
  assert_command_execution_succeeds(env, "np.bitwise_and(np.array([1,2], dtype=np.uint16), np.array([3,4], dtype=np.uint16))");
  assert_command_execution_succeeds(env, "np.bitwise_or(np.array([1,2], dtype=np.uint16), np.array([3,4], dtype=np.uint16))");
  assert_command_execution_succeeds(env, "np.bitwise_xor(np.array([1,2], dtype=np.uint16), np.array([3,4], dtype=np.uint16))");
  assert_command_execution_succeeds(env, "np.left_shift(np.array([1,2], dtype=np.uint16), 1)");
  assert_command_execution_succeeds(env, "np.right_shift(np.array([2,4], dtype=np.uint16), 1)");
  assert_command_execution_succeeds(env, "np.random.Generator(None).random()");
  assert_command_execution_succeeds(env, "np.random.Generator(None).normal()");
  assert_command_execution_succeeds(env, "np.random.Generator(None).uniform(0, 1)");
  assert_command_execution_succeeds(env, "spy.integrate.tanhsinh(lambda x: x, 0, 1)");
  assert_command_execution_succeeds(env, "spy.integrate.romberg(lambda x: x, 0, 1)");
  assert_command_execution_succeeds(env, "spy.integrate.simpson(lambda x: x**2 + 2*x + 1, 0, 5)");
  assert_command_execution_succeeds(env, "spy.integrate.quad(lambda x: x, 0, 1)");
  assert_command_execution_fails(env, "spy.integrate.quad(lambda x: x, 0)");
  assert_command_execution_fails(env, "spy.integrate.simpson(1)");
  assert_command_execution_succeeds(env, "np.sum(np.array([1,2,3]), axis=0)");
  assert_command_execution_succeeds(env, "np.mean(np.array([[1,2],[3,4]]), axis=1)");
  assert_command_execution_succeeds(env, "np.polyfit(np.array([1,2,3]), np.array([1,2,3]), 1)");
  assert_command_execution_succeeds(env, "np.sum(np.array([1,2,3]), keepdims=5)");
  assert_command_execution_fails(env, "np.mean(np.array([1,2,3]), axis=5)");

  deinit_environment();
}

