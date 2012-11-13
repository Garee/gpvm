#include <stdio.h>
#include <stdlib.h>
#include "minunit.h"

#define KERNEL_TEST_ENABLED
#include "../kernels/vm.cl" 

int tests_run = 0;
 
/* Helper Functions. */
packet *q_create() {
  packet *q = malloc((16 + (QUEUE_SIZE * 16)) * sizeof(packet));
  if (q) {
    for (int i = 0; i < (16 + (QUEUE_SIZE * 16)); i++) {
      q[i].x = 0;
      q[i].y = 0;
    }
  }

  return q;
}

void q_destroy(packet *q) {
  free(q);
}

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

static char *test_q_get_head_index() {
  packet *q = q_create();
  packet i;
  mu_assert("FAIL: test_q_get_head_index [1]", q_get_head_index(0, 0, q, 4) == 0);
  q_write(i, 0, q, 4);
  q_read(&i, 0, q, 4);
  mu_assert("FAIL: test_q_get_head_index [2]", q_get_head_index(0, 0, q, 4) == 1);
  q_destroy(q);
  return NULL;
}

static char *test_q_get_tail_index() {
  packet *q = q_create();
  packet i; 
  mu_assert("FAIL: test_q_get_tail_index [1]", q_get_tail_index(0, 0, q, 4) == 0);
  q_write(i, 1, q, 4);
  q_write(i, 1, q, 4);
  mu_assert("FAIL: test_q_get_tail_index [2]", q_get_tail_index(1, 0, q, 4) == 2);
  mu_assert("FAIL: test_q_get_tail_index [3]", q_get_tail_index(0, 0, q, 4) == 0);
  q_write(i, 0, q, 4);
  mu_assert("FAIL: test_q_get_tail_index [4]", q_get_tail_index(0, 0, q, 4) == 1);
  q_destroy(q);
  return NULL;
}

static char *test_q_set_head_index() {
  packet *q = q_create();
  q_set_head_index(10, 0, 0, q, 4);
  mu_assert("FAIL: test_q_set_head_index [1]", q_get_head_index(0, 0, q, 4) == 10);
  q_set_head_index(0, 0, 0, q, 4);
  mu_assert("FAIL: test_q_set_head_index [2]", q_get_head_index(0, 0, q, 4) == 0);
  mu_assert("FAIL: test_q_set_head_index [3]", q_get_head_index(1, 0, q, 4) == 0);
  q_destroy(q);
  return NULL;
}

static char *test_q_set_tail_index() {
  packet *q = q_create();
  q_set_tail_index(10, 0, 0, q, 4);
  mu_assert("FAIL: test_q_set_tail_index [1]", q_get_tail_index(0, 0, q, 4) == 10);
  q_set_tail_index(0, 0, 0, q, 4);
  mu_assert("FAIL: test_q_set_tail_index [2]", q_get_tail_index(0, 0, q, 4) == 0);
  mu_assert("FAIL: test_q_set_tail_index [3]", q_get_tail_index(1, 0, q, 4) == 0);
  q_destroy(q);
  return NULL;
}

static char *test_q_set_last_op() {
  return NULL;
}

static char *test_q_last_op_is_read() {
  return NULL;
}

static char *test_q_last_op_is_write() {
  return NULL;
}

static char *test_q_is_empty() {
  return NULL;
}

static char *test_q_is_full() {
  return NULL;
}

static char *test_q_size() {
  return NULL;
}

static char *test_q_read() {
  return NULL;
}

static char *test_q_write() {
  return NULL;
}

static char *test_cunit_q_is_empty() {
  return NULL;
}
static char *test_cunit_q_is_full() {
  return NULL;
}

static char *test_cunit_q_size() {
  return NULL;
}

static char *test_transferRQ() {
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

  mu_run_test(test_q_get_head_index);
  mu_run_test(test_q_get_tail_index);
  mu_run_test(test_q_set_head_index);
  mu_run_test(test_q_set_tail_index);
  mu_run_test(test_q_set_last_op);
  mu_run_test(test_q_last_op_is_read);
  mu_run_test(test_q_last_op_is_write);
  mu_run_test(test_q_is_empty);
  mu_run_test(test_q_is_full);
  mu_run_test(test_q_size);
  mu_run_test(test_q_read);
  mu_run_test(test_q_write);

  mu_run_test(test_cunit_q_is_empty);
  mu_run_test(test_cunit_q_is_full);
  mu_run_test(test_cunit_q_size);
  mu_run_test(test_transferRQ);
  return NULL;
}
 
int main(void) {
  char *result = all_tests();
  if (result) {
    printf("%s\n", result);
  }
  else {
    printf("ALL TESTS PASSED\n");
  }

  printf("Tests run: %d\n", tests_run);
  return result != 0;
}

