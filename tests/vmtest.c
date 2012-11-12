#include <stdio.h>
#include "minunit.h"

#define KERNEL_TEST_ENABLED
#include "../kernels/vm.cl" 

int tests_run = 0;
 
static char *test_foo() {
  return NULL;
}
 
static char *test_bar() {
  return NULL;
}
 
static char *all_tests() {
  mu_run_test(test_foo);
  mu_run_test(test_bar);
  return NULL;
}
 
int main(void) {
  char *result = all_tests();
  if (result != 0) {
    printf("%s\n", result);
  }
  else {
    printf("ALL TESTS PASSED\n");
  }

  printf("Tests run: %d\n", tests_run);
  return result != 0;
}
