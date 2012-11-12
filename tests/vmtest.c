#include <stdio.h>
#include "minunit.h"

#define KERNEL_TEST_ENABLED
#include "../kernels/vm.cl" 

int tests_run = 0;
 
static char *test_pkt_get_type() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  mu_assert("FAIL: test_pkt_get_type [1]", pkt_get_type(p) == REFERENCE);
  mu_assert("FAIL: test_pkt_get_type [2]", pkt_get_type(p) != ERROR);
  return NULL;
}

static char *test_pkt_get_dest() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  mu_assert("FAIL: test_pkt_get_dest [2]", pkt_get_dest(p) != 2);
  mu_assert("FAIL: test_pkt_get_dest [1]", pkt_get_dest(p) == 1);
  return NULL;
}

static char *test_pkt_get_arg() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  mu_assert("FAIL: test_pkt_get_arg [2]", pkt_get_arg(p) != 3);
  mu_assert("FAIL: test_pkt_get_arg [1]", pkt_get_arg(p) == 2);
  return NULL;
}

static char *test_pkt_get_sub() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  mu_assert("FAIL: test_pkt_get_sub [2]", pkt_get_sub(p) != 4);
  mu_assert("FAIL: test_pkt_get_sub [1]", pkt_get_sub(p) == 3);
  return NULL;
}

static char *test_pkt_get_payload() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  mu_assert("FAIL: test_pkt_get_payload [2]", pkt_get_payload(p) != 1);
  mu_assert("FAIL: test_pkt_get_payload [1]", pkt_get_payload(p) == 4);
  return NULL;
}

static char *test_pkt_set_type() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  pkt_set_type(&p, ERROR);
  mu_assert("FAIL: test_pkt_set_type [2]", pkt_get_type(p) != REFERENCE);
  mu_assert("FAIL: test_pkt_set_type [1]", pkt_get_type(p) == ERROR);
  return NULL;
}

static char *test_pkt_set_dest() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  pkt_set_dest(&p, 7);
  mu_assert("FAIL: test_pkt_set_dest [2]", pkt_get_dest(p) != 2);
  mu_assert("FAIL: test_pkt_set_dest [1]", pkt_get_dest(p) == 7);
  return NULL;
}

static char *test_pkt_set_arg() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  pkt_set_arg(&p, 7);
  mu_assert("FAIL: test_pkt_set_arg [2]", pkt_get_arg(p) != 3);
  mu_assert("FAIL: test_pkt_set_arg [1]", pkt_get_arg(p) == 7);
  return NULL;
}

static char *test_pkt_set_sub() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  pkt_set_sub(&p, 7);
  mu_assert("FAIL: test_pkt_set_sub [2]", pkt_get_sub(p) != 4);
  mu_assert("FAIL: test_pkt_set_sub [1]", pkt_get_sub(p) == 7);
  return NULL;
}

static char *test_pkt_set_payload() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  pkt_set_payload(&p, 7);
  mu_assert("FAIL: test_pkt_set_payload [2]", pkt_get_payload(p) != 1);
  mu_assert("FAIL: test_pkt_set_payload [1]", pkt_get_payload(p) == 7);
  return NULL;
}

static char *test_pkt_create() {
  packet p = pkt_create(ERROR, 1, 2, 3, 4);
  mu_assert("FAIL: test_pkt_create [1]", pkt_get_type(p) == ERROR);
  mu_assert("FAIL: test_pkt_create [2]", pkt_get_dest(p) == 1);
  mu_assert("FAIL: test_pkt_create [3]", pkt_get_arg(p) == 2);
  mu_assert("FAIL: test_pkt_create [4]", pkt_get_sub(p) == 3);
  mu_assert("FAIL: test_pkt_create [5]", pkt_get_payload(p) == 4);
  return NULL;
}
 
static char *all_tests() {
  mu_run_test(test_pkt_get_type);
  mu_run_test(test_pkt_get_dest);
  mu_run_test(test_pkt_get_arg);
  mu_run_test(test_pkt_get_sub);
  mu_run_test(test_pkt_get_payload);
  mu_run_test(test_pkt_set_type);
  mu_run_test(test_pkt_set_dest);
  mu_run_test(test_pkt_set_arg);
  mu_run_test(test_pkt_set_sub);
  mu_run_test(test_pkt_set_payload);
  mu_run_test(test_pkt_create);
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
