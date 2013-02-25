#include <stdio.h>
#include <stdlib.h>
#include "minunit.h"

#define KERNEL_TEST_ENABLED
#include "../kernels/vm.cl"

#define N 4 // Number of compute units.

int tests_run = 0;

void printB(ulong n) {
  while (n) {
    if (n & 1)
      printf("1");
    else
      printf("0");
    
    n >>= 1;
  }
  printf("\n");
}

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

subt_rec *subt_rec_create(uint service_id) {
  subt_rec *rec = malloc(sizeof(subt_rec));
  if (rec) {
    subt_rec_set_service_id(rec, service_id);
    subt_rec_set_subt_status(rec, NEW);
    
    for (int i = 0; i < QUEUE_SIZE; i++) {
      subt_rec_set_arg(rec, i, 0);
    }
    
    subt_rec_set_nargs_absent(rec, 1);
    subt_rec_set_return_to(rec, 2);
    subt_rec_set_return_as(rec, 3);
  }
  
  return rec;
}

void subt_rec_destroy(subt_rec *rec) {
  free(rec);
}

subt *subt_create() {
  subt *subt = malloc(sizeof(subt_rec) * SUBT_SIZE + sizeof(ushort) * SUBT_SIZE + 1);
  if (subt) {
    subt->av_recs[0] = 1;
    for (int i = 1; i < SUBT_SIZE + 1; i++) {
      subt->av_recs[i] = i - 1;
    }
  }

  return subt;
}

void subt_destroy(subt *subt) {
  free(subt);
}

/* Unit Tests */
static char *test_pkt_get_type() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  mu_assert("FAIL: test_pkt_get_type [1]", pkt_get_type(p) == REFERENCE);
  return NULL;
}

static char *test_pkt_get_source() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  mu_assert("FAIL: test_pkt_get_source [1]", pkt_get_source(p) == 1);
  return NULL;
}

static char *test_pkt_get_arg_pos() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  mu_assert("FAIL: test_pkt_get_arg_pos [1]", pkt_get_arg_pos(p) == 2);
  return NULL;
}

static char *test_pkt_get_sub() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  mu_assert("FAIL: test_pkt_get_sub [1]", pkt_get_sub(p) == 3);
  return NULL;
}

static char *test_pkt_get_payload_type() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  mu_assert("FAIL: test_pkt_get_payload_type [1]", pkt_get_payload_type(p) == VAL);
  pkt_set_payload_type(&p, PTR);
  mu_assert("FAIL: test_pkt_get_payload_type [2]", pkt_get_payload_type(p) == PTR);
  return NULL;
}

static char *test_pkt_get_payload() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  mu_assert("FAIL: test_pkt_get_payload [1]", pkt_get_payload(p) == 4);
  return NULL;
}

static char *test_pkt_set_type() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  pkt_set_type(&p, ERROR);
  mu_assert("FAIL: test_pkt_set_type [1]", pkt_get_type(p) == ERROR);
  return NULL;
}

static char *test_pkt_set_source() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  pkt_set_source(&p, 7);
  mu_assert("FAIL: test_pkt_set_source [1]", pkt_get_source(p) == 7);
  return NULL;
}

static char *test_pkt_set_arg_pos() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  pkt_set_arg_pos(&p, 7);
  mu_assert("FAIL: test_pkt_set_arg_pos [1]", pkt_get_arg_pos(p) == 7);
  return NULL;
}

static char *test_pkt_set_sub() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  pkt_set_sub(&p, 7);
  mu_assert("FAIL: test_pkt_set_sub [1]", pkt_get_sub(p) == 7);
  return NULL;
}

static char *test_pkt_set_payload_type() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  pkt_set_payload_type(&p, PTR);
  mu_assert("FAIL: test_pkt_set_payload_type [1]", pkt_get_payload_type(p) == PTR);
  return NULL;
}

static char *test_pkt_set_payload() {
  packet p = pkt_create(REFERENCE, 1, 2, 3, 4);
  pkt_set_payload(&p, 7);
  mu_assert("FAIL: test_pkt_set_payload [1]", pkt_get_payload(p) == 7);
  return NULL;
}

static char *test_pkt_create() {
  packet p = pkt_create(ERROR, 1, 2, 3, 4);
  mu_assert("FAIL: test_pkt_create [1]", pkt_get_type(p) == ERROR);
  mu_assert("FAIL: test_pkt_create [2]", pkt_get_source(p) == 1);
  mu_assert("FAIL: test_pkt_create [3]", pkt_get_arg_pos(p) == 2);
  mu_assert("FAIL: test_pkt_create [4]", pkt_get_sub(p) == 3);
  mu_assert("FAIL: test_pkt_create [5]", pkt_get_payload_type(p) == VAL);
  mu_assert("FAIL: test_pkt_create [6]", pkt_get_payload(p) == 4);
  return NULL;
}

static char *test_q_get_head_index() {
  packet *q = q_create();
  packet i;
  mu_assert("FAIL: test_q_get_head_index [1]", q_get_head_index(0, 0, q, N) == 0);
  q_write(i, 0, q, N);
  q_read(&i, 0, q, N);
  mu_assert("FAIL: test_q_get_head_index [2]", q_get_head_index(0, 0, q, N) == 1);
  q_destroy(q);
  return NULL;
}

static char *test_q_get_tail_index() {
  packet *q = q_create();
  packet i;
  mu_assert("FAIL: test_q_get_tail_index [1]", q_get_tail_index(0, 0, q, N) == 0);
  q_write(i, 1, q, N);
  q_write(i, 1, q, N);
  mu_assert("FAIL: test_q_get_tail_index [2]", q_get_tail_index(1, 0, q, N) == 2);
  mu_assert("FAIL: test_q_get_tail_index [3]", q_get_tail_index(0, 0, q, N) == 0);
  q_write(i, 0, q, N);
  mu_assert("FAIL: test_q_get_tail_index [4]", q_get_tail_index(0, 0, q, N) == 1);
  q_destroy(q);
  return NULL;
}

static char *test_q_set_head_index() {
  packet *q = q_create();
  q_set_head_index(10, 0, 0, q, N);
  mu_assert("FAIL: test_q_set_head_index [1]", q_get_head_index(0, 0, q, N) == 10);
  q_set_head_index(0, 0, 0, q, N);
  mu_assert("FAIL: test_q_set_head_index [2]", q_get_head_index(0, 0, q, N) == 0);
  mu_assert("FAIL: test_q_set_head_index [3]", q_get_head_index(1, 0, q, N) == 0);
  q_destroy(q);
  return NULL;
}

static char *test_q_set_tail_index() {
  packet *q = q_create();
  q_set_tail_index(10, 0, 0, q, N);
  mu_assert("FAIL: test_q_set_tail_index [1]", q_get_tail_index(0, 0, q, N) == 10);
  q_set_tail_index(0, 0, 0, q, N);
  mu_assert("FAIL: test_q_set_tail_index [2]", q_get_tail_index(0, 0, q, N) == 0);
  mu_assert("FAIL: test_q_set_tail_index [3]", q_get_tail_index(1, 0, q, N) == 0);
  q_destroy(q);
  return NULL;
}

static char *test_q_set_last_op() {
  packet *q = q_create();
  q_set_last_op(WRITE, 0, 0, q, N);
  mu_assert("FAIL: test_q_set_last_op [1]", q_last_op_is_write(0, 0, q, N));
  q_set_last_op(READ, 4, 0, q, N);
  mu_assert("FAIL: test_q_set_last_op [2]", !q_last_op_is_write(4, 0, q, N));
  mu_assert("FAIL: test_q_set_last_op [3]", q_last_op_is_read(4, 0, q, N));
  q_destroy(q);
  return NULL;
}

static char *test_q_last_op_is_read() {
  packet *q = q_create();
  q_set_last_op(READ, 0, 0, q, N);
  mu_assert("FAIL: test_q_last_op_is_read [1]", q_last_op_is_read(0, 0, q, N));
  q_set_last_op(READ, 1, 0, q, N);
  mu_assert("FAIL: test_q_last_op_is_read [2]", q_last_op_is_read(1, 0, q, N));
  q_set_last_op(READ, 2, 0, q, N);
  mu_assert("FAIL: test_q_last_op_is_read [3]", q_last_op_is_read(2, 0, q, N));
  q_set_last_op(WRITE, 3, 0, q, N);
  mu_assert("FAIL: test_q_last_op_is_read [4]", !q_last_op_is_read(3, 0, q, N));
  q_destroy(q);
  return NULL;
}

static char *test_q_last_op_is_write() {
  packet *q = q_create();
  q_set_last_op(WRITE, 0, 0, q, N);
  mu_assert("FAIL: test_q_last_op_is_write [1]", q_last_op_is_write(0, 0, q, N));
  q_set_last_op(WRITE, 1, 0, q, N);
  mu_assert("FAIL: test_q_last_op_is_write [2]", q_last_op_is_write(1, 0, q, N));
  q_set_last_op(WRITE, 2, 0, q, N);
  mu_assert("FAIL: test_q_last_op_is_write [3]", q_last_op_is_write(2, 0, q, N));
  q_set_last_op(READ, 3, 0, q, N);
  mu_assert("FAIL: test_q_last_op_is_write [4]", !q_last_op_is_write(3, 0, q, N));
  q_destroy(q);
  return NULL;
}

static char *test_q_is_empty() {
  packet *q = q_create();
  packet p = pkt_create(ERROR, 0, 0, 0, 0);
  mu_assert("FAIL: test_q_is_empty [1]", q_is_empty(0, 0, q, N));
  q_write(p, 0, q, N);
  mu_assert("FAIL: test_q_is_empty [2]", !q_is_empty(0, 0, q, N));
  q_read(&p, 0, q, N);
  mu_assert("FAIL: test_q_is_empty [3]", q_is_empty(0, 0, q, N));
  q_destroy(q);
  return NULL;
}

static char *test_q_is_full() {
  packet *q = q_create();
  packet p = pkt_create(ERROR, 0, 0, 0, 0);
  mu_assert("FAIL: test_q_is_full [1]", !q_is_full(0, 0, q, N));
  for (int i = 0; i < 16; i++) {
    q_write(p, 0, q, N);
  }
  mu_assert("FAIL: test_q_is_full [2]", q_is_full(0, 0, q, N));
  q_read(&p, 0, q, N);
  mu_assert("FAIL: test_q_is_full [3]", !q_is_full(0, 0, q, N));
  q_destroy(q);
  return NULL;
}

static char *test_q_size() {
  packet *q = q_create();
  packet p = pkt_create(ERROR, 0, 0, 0, 0);
  mu_assert("FAIL: test_q_size [1]", q_size(0, 0, q, N) == 0);
  for (int i = 0; i < 20; i++) {
    q_write(p, 0, q, N);
  }
  mu_assert("FAIL: test_q_size [2]", q_size(0, 0, q, N) == 16);
  q_read(&p, 0, q, N);
  mu_assert("FAIL: test_q_size [3]", q_size(0, 0, q, N) == 15);
  q_destroy(q);
  return NULL;
}

static char *test_q_read() {
  packet *q = q_create();
  packet p = pkt_create(REFERENCE, 0, 0, 0, 0);
  packet p2 = pkt_create(ERROR, 0, 0, 0, 0);
  q_write(p, 0, q, N);
  q_read(&p2, 0, q, N);
  mu_assert("FAIL: test_q_read [1]", (p.x == p2.x) && (p.y == p2.y));
  q_read(&p2, 0, q, N);
  mu_assert("FAIL: test_q_read [2]", !q_read(&p2, 0, q, N));
  q_destroy(q);
  return NULL;
}

static char *test_q_write() {
  packet *q = q_create();
  packet p = pkt_create(DATA, 1, 2, 3, 4);
  q_write(p, 0, q, N);
  mu_assert("FAIL: test_q_write [1]", !q_is_empty(0, 0, q, N));
  q_write(p, 0, q, N);
  mu_assert("FAIL: test_q_write [2]", q_size(0, 0, q, N) == 2);
  for (int i = 0; i < 14; i++) {
    q_write(p, 0, q, N);
  }
  mu_assert("FAIL: test_q_write [3]", !q_write(p, 0, q, N));
  q_destroy(q);
  return NULL;
}

static char *test_transferRQ() {
  packet *q = q_create();
  packet *rq = q_create();
  packet p = pkt_create(DATA, 1, 2, 3, 4);
  packet p2;
  q_write(p, 0, rq, 4);
  transferRQ(rq, q, 4);
  q_read(&p2, 0, q, 4);
  mu_assert("FAIL: test_transferRQ", (p.x == p2.x) && (p.y == p2.y));
  q_destroy(q);
  q_destroy(rq);
  return NULL;
}

static char *test_subt_rec_get_service_id() {
  subt_rec *rec = subt_rec_create(1);
  mu_assert("FAIL: test_subt_rec_get_service_id", subt_rec_get_service_id(rec) == 1);
  subt_rec_destroy(rec);
  return NULL;
}

static char *test_subt_rec_get_arg() {
  subt_rec *rec = subt_rec_create(1);
  mu_assert("FAIL: test_subt_rec_get_arg", subt_rec_get_arg(rec, 0) == 0);
  subt_rec_destroy(rec);
  return NULL;
}

static char *test_subt_rec_get_subt_status() {
  subt_rec *rec = subt_rec_create(1);
  mu_assert("FAIL: test_subt_rec_get_subt_status", subt_rec_get_subt_status(rec) == NEW);
  subt_rec_destroy(rec);
  return NULL;
}

static char *test_subt_rec_get_nargs_absent() {
  subt_rec *rec = subt_rec_create(1);
  mu_assert("FAIL: test_subt_rec_get_nargs_absent", subt_rec_get_nargs_absent(rec) == 1);
  subt_rec_destroy(rec);
  return NULL;
}

static char *test_subt_rec_get_return_to() {
  subt_rec *rec = subt_rec_create(1);
  mu_assert("FAIL: test_subt_rec_get_return_to", subt_rec_get_return_to(rec) == 2);
  subt_rec_destroy(rec);
  return NULL;
}

static char *test_subt_rec_get_return_as() {
  subt_rec *rec = subt_rec_create(1);
  mu_assert("FAIL: test_subt_rec_get_return_as", subt_rec_get_return_as(rec) == 3);
  subt_rec_destroy(rec);
  return NULL;
}

static char *test_subt_rec_set_service_id() {
  subt_rec *rec = subt_rec_create(1);
  subt_rec_set_service_id(rec, 2);
  mu_assert("FAIL: test_subt_rec_set_service_id", subt_rec_get_service_id(rec) == 2);
  subt_rec_destroy(rec);
  return NULL;
}

static char *test_subt_rec_set_arg() {
  subt_rec *rec = subt_rec_create(1);
  subt_rec_set_arg(rec, 0, 50);
  mu_assert("FAIL: test_subt_rec_set_arg", subt_rec_get_arg(rec, 0) == 50);
  subt_rec_destroy(rec);
  return NULL;
}

static char *test_subt_rec_set_subt_status() {
  subt_rec *rec = subt_rec_create(1);
  subt_rec_set_subt_status(rec, PROCESSING);
  mu_assert("FAIL: test_subt_rec_set_subt_status", subt_rec_get_subt_status(rec) == PROCESSING);
  subt_rec_destroy(rec);
  return NULL;
}

static char *test_subt_rec_set_nargs_absent() {
  subt_rec *rec = subt_rec_create(1);
  subt_rec_set_nargs_absent(rec, 4);
  mu_assert("FAIL: test_subt_rec_set_nargs_absent", subt_rec_get_nargs_absent(rec) == 4);
  subt_rec_destroy(rec);
  return NULL;
}

static char *test_subt_rec_set_return_to() {
  subt_rec *rec = subt_rec_create(1);
  subt_rec_set_return_to(rec, 100);
  mu_assert("FAIL: test_subt_rec_set_return_to", subt_rec_get_return_to(rec) == 100);
  subt_rec_destroy(rec);
  return NULL;
}

static char *test_subt_rec_set_return_as() {
  subt_rec *rec = subt_rec_create(1);
  subt_rec_set_return_as(rec, 255);
  mu_assert("FAIL: test_subt_rec_set_return_as", subt_rec_get_return_as(rec) == 255);
  subt_rec_destroy(rec);
  return NULL;
}

static char *test_subt_is_full() {
  subt *subt = subt_create();
  mu_assert("FAIL: test_subt_is_full", !subt_is_full(subt));
  
  ushort av_index;
  for (int i = 0; i < SUBT_SIZE; i++) {
    subt_pop(&av_index, subt);
  }
  
  mu_assert("FAIL: test_subt_is_full", subt_is_full(subt));
  subt_destroy(subt);
  return NULL;
}

static char *test_subt_is_empty() {
  subt *subt = subt_create();
  mu_assert("FAIL: test_subt_is_empty", subt_is_empty(subt));
  subt_destroy(subt);
  return NULL;
}

static char *test_subt_top() {
  subt *subt = subt_create();
  mu_assert("FAIL: test_subt_top", subt_top(subt) == 1);
  subt_destroy(subt);
  return NULL;
}

static char *test_subt_set_top() {
  subt *subt = subt_create();
  subt_set_top(subt, 7);
  mu_assert("FAIL: test_subt_set_top", subt_top(subt) == 7);
  subt_destroy(subt);
  return NULL;
}

static char *test_subt_push() {
  subt *subt = subt_create();
  mu_assert("FAIL: test_subt_push [1]", !subt_push(0, subt));
  ushort i;
  subt_pop(&i, subt);
  mu_assert("FAIL: test_subt_push [2]", subt_push(0, subt));
  mu_assert("FAIL: test_subt_push [3]", !subt_push(1, subt));
  subt_destroy(subt);
  return NULL;
}

static char *test_subt_pop() {
  subt *subt = subt_create();
  ushort i = 0;
  mu_assert("FAIL: test_subt_pop [1]", subt_pop(&i, subt));
  mu_assert("FAIL: test_subt_pop [2]", subt_pop(&i, subt));
  mu_assert("FAIL: test_subt_pop [3]", i == 1);
  subt_destroy(subt);
  return NULL;
}

static char *test_subt_get_rec() {
  subt *subt = subt_create();
  for (int i = 0; i < SUBT_SIZE; i++) {
    mu_assert("FAIL: test_subt_get_rec", subt_get_rec(i, subt) != NULL);
  }
  subt_destroy(subt);
  return NULL;
}

static char *test_symbol_KS_create() {
  bytecode s = symbol_KS_create(1, 1, 2, 3, 4);
  mu_assert("FAIL: test_symbol_KS_create [1]", symbol_get_kind(s) == K_S);
  mu_assert("FAIL: test_symbol_KS_create [2]", !symbol_is_quoted(s));
  mu_assert("FAIL: test_symbol_KS_create [3]", symbol_get_nargs(s) == 1);
  mu_assert("FAIL: test_symbol_KS_create [4]", symbol_get_SNId(s) == 1);
  return NULL;
}

static char *test_symbol_KR_create() {
  bytecode s = symbol_KR_create(1, 1);
  mu_assert("FAIL: test_symbol_KR_create [1]", symbol_get_kind(s) == K_R);
  mu_assert("FAIL: test_symbol_KR_create [2]", !symbol_is_quoted(s));
  mu_assert("FAIL: test_symbol_KR_create [3]", symbol_get_subtask(s) == 1);
  mu_assert("FAIL: test_symbol_KR_create [4]", symbol_get_value(s) == 1);
  return NULL;
}

static char *test_symbol_KB_create() {
  bytecode s = symbol_KB_create(1);
  mu_assert("FAIL: test_symbol_KB_create [1]", symbol_get_kind(s) == K_B);
  mu_assert("FAIL: test_symbol_KB_create [2]", symbol_is_quoted(s));
  mu_assert("FAIL: test_symbol_KB_create [3]", symbol_get_value(s) == 1);
  return NULL;
}

static char *test_symbol_get_kind() {
  bytecode s = symbol_KS_create(1, 1, 2, 3, 4);
  mu_assert("FAIL: test_symbol_get_kind [1]", symbol_get_kind(s) == K_S);
  return NULL;
}

static char *test_symbol_is_quoted() {
  bytecode s = symbol_KB_create(1);
  mu_assert("FAIL: test_symbol_is_quoted [1]", symbol_is_quoted(s));
  return NULL;
}

static char *test_symbol_get_service() {
  bytecode s = symbol_KS_create(1, 0, 0, 0, 1);
  mu_assert("FAIL: test_symbol_get_service [1]", symbol_get_service(s) == 1);
  return NULL;
}

static char *test_symbol_get_SNId() {
  bytecode s = symbol_KS_create(2, 3, 4, 5, 6);
  mu_assert("FAIL: test_symbol_get_SNId [1]", symbol_get_SNId(s) == 3);
  return NULL;
}

static char *test_symbol_get_SNLId() {
  bytecode s = symbol_KS_create(2, 3, 4, 5, 6);
  mu_assert("FAIL: test_symbol_get_SNLId [1]", symbol_get_SNLId(s) == 4);
  return NULL;
}

static char *test_symbol_get_SNCId() {
  bytecode s = symbol_KS_create(2, 3, 4, 5, 6);
  mu_assert("FAIL: test_symbol_get_SNCId [1]", symbol_get_SNCId(s) == 5);
  return NULL;
}

static char *test_symbol_get_opcode() {
  bytecode s = symbol_KS_create(2, 3, 4, 5, 6);
  mu_assert("FAIL: test_symbol_get_opcode [1]", symbol_get_opcode(s) == 6);
  return NULL;
}

static char *test_symbol_get_subtask() {
  bytecode s = symbol_KR_create(2, 3);
  mu_assert("FAIL: test_symbol_get_subtask [1]", symbol_get_subtask(s) == 2);
  return NULL;
}

static char *test_symbol_get_nargs() {
  bytecode s = symbol_KS_create(5, 1, 2, 3, 4);
  mu_assert("FAIL: test_symbol_get_nargs [1]", symbol_get_nargs(s) == 5);
  return NULL;
}

static char *test_symbol_get_value() {
  bytecode s = symbol_KB_create(7);
  mu_assert("FAIL: test_symbol_get_value [1]", symbol_get_value(s) == 7);
  return NULL;
}

static char *test_symbol_set_kind() {
  bytecode s = symbol_KS_create(1, 1, 2, 3, 4);
  symbol_set_kind(&s, K_R);
  mu_assert("FAIL: test_symbol_set_kind [1]", symbol_get_kind(s) == K_R);
  return NULL;
}

static char *test_symbol_quote() {
  bytecode s = symbol_KS_create(1, 1, 2, 3, 4);
  symbol_quote(&s);
  mu_assert("FAIL: test_symbol_quote [1]", symbol_is_quoted(s));
  return NULL;
}

static char *test_symbol_unquote() {
  bytecode s = symbol_KS_create(1, 1, 2, 3, 4);
  symbol_quote(&s);
  symbol_unquote(&s);
  mu_assert("FAIL: test_symbol_unquote [1]", !symbol_is_quoted(s));
  return NULL;
}

static char *test_symbol_set_service() {
  bytecode s = symbol_KS_create(1, 1, 2, 3, 4);
  symbol_set_service(&s, 5);
  mu_assert("FAIL: test_symbol_set_service [1]", symbol_get_service(s) == 5);
  return NULL;
}

static char *test_symbol_set_SNId() {
  bytecode s = symbol_KS_create(2, 1, 2, 3, 4);
  symbol_set_SNId(&s, 4);
  mu_assert("FAIL: test_symbol_set_SNId [1]", symbol_get_SNId(s) == 4);
  return NULL;
}

static char *test_symbol_set_SNLId() {
  bytecode s = symbol_KS_create(2, 1, 2, 3, 4);
  symbol_set_SNLId(&s, 4);
  mu_assert("FAIL: test_symbol_set_SNLId [1]", symbol_get_SNLId(s) == 4);
  return NULL;
}

static char *test_symbol_set_SNCId() {
  bytecode s = symbol_KS_create(2, 1, 2, 3, 4);
  symbol_set_SNCId(&s, 4);
  mu_assert("FAIL: test_symbol_set_SNCId [1]", symbol_get_SNCId(s) == 4);
  return NULL;
}

static char *test_symbol_set_opcode() {
  bytecode s = symbol_KS_create(2, 1, 2, 3, 5);
  symbol_set_opcode(&s, 4);
  mu_assert("FAIL: test_symbol_set_opcode [1]", symbol_get_opcode(s) == 4);
  return NULL;
}

static char *test_symbol_set_subtask() {
  bytecode s = symbol_KR_create(2, 3);
  symbol_set_subtask(&s, 5);
  mu_assert("FAIL: test_symbol_set_subtask [1]", symbol_get_subtask(s) == 5);
  return NULL;
}

static char *test_symbol_set_nargs() {
  bytecode s = symbol_KS_create(1, 1, 2, 3, 4);
  symbol_set_nargs(&s, 3);
  mu_assert("FAIL: test_symbol_set_nargs [1]", symbol_get_nargs(s) == 3);
  return NULL;
}

static char *test_symbol_set_value() {
  bytecode s = symbol_KB_create(1);
  symbol_set_value(&s, 10);
  mu_assert("FAIL: test_symbol_set_value [1]", symbol_get_value(s) == 10);
  return NULL;
}

static char *all_tests() {
  mu_run_test(test_pkt_get_type);
  mu_run_test(test_pkt_get_source);
  mu_run_test(test_pkt_get_arg_pos);
  mu_run_test(test_pkt_get_sub);
  mu_run_test(test_pkt_get_payload_type);
  mu_run_test(test_pkt_get_payload);
  mu_run_test(test_pkt_set_type);
  mu_run_test(test_pkt_set_source);
  mu_run_test(test_pkt_set_arg_pos);
  mu_run_test(test_pkt_set_sub);
  mu_run_test(test_pkt_set_payload_type);
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

  mu_run_test(test_transferRQ);
  
  mu_run_test(test_subt_rec_get_service_id);
  mu_run_test(test_subt_rec_get_arg);
  mu_run_test(test_subt_rec_get_subt_status);
  mu_run_test(test_subt_rec_get_nargs_absent);
  mu_run_test(test_subt_rec_get_return_to);
  mu_run_test(test_subt_rec_get_return_as);
  mu_run_test(test_subt_rec_set_service_id);
  mu_run_test(test_subt_rec_set_arg);
  mu_run_test(test_subt_rec_set_subt_status);
  mu_run_test(test_subt_rec_set_nargs_absent);
  mu_run_test(test_subt_rec_set_return_to);
  mu_run_test(test_subt_rec_set_return_as);

  mu_run_test(test_subt_is_full);
  mu_run_test(test_subt_is_empty);
  mu_run_test(test_subt_top);
  mu_run_test(test_subt_set_top);
  mu_run_test(test_subt_push);
  mu_run_test(test_subt_pop);
  mu_run_test(test_subt_get_rec);
  
  mu_run_test(test_symbol_KS_create);
  mu_run_test(test_symbol_KR_create);
  mu_run_test(test_symbol_KB_create);
  mu_run_test(test_symbol_get_kind);
  mu_run_test(test_symbol_is_quoted);
  mu_run_test(test_symbol_get_service);
  mu_run_test(test_symbol_get_SNId);
  mu_run_test(test_symbol_get_SNLId);
  mu_run_test(test_symbol_get_SNCId);
  mu_run_test(test_symbol_get_opcode);
  mu_run_test(test_symbol_get_subtask);
  mu_run_test(test_symbol_get_nargs);
  mu_run_test(test_symbol_get_value);
  mu_run_test(test_symbol_set_kind);
  mu_run_test(test_symbol_quote);
  mu_run_test(test_symbol_unquote);
  mu_run_test(test_symbol_set_service);
  mu_run_test(test_symbol_set_SNId);
  mu_run_test(test_symbol_set_SNLId);
  mu_run_test(test_symbol_set_SNCId);
  mu_run_test(test_symbol_set_opcode);
  mu_run_test(test_symbol_set_subtask);
  mu_run_test(test_symbol_set_nargs);
  mu_run_test(test_symbol_set_value);
  
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
 
