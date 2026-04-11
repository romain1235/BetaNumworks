#include <quiz.h>
#include "execution_environment.h"

QUIZ_CASE(python_console_module) {
  TestExecutionEnvironment env = init_environnement();
  // Import should succeed without raising
  assert_command_execution_succeeds(env, "import console","\n");
  deinit_environment();
}
