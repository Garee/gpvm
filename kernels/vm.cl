#define READ 0
#define WRITE 1

uint q_get_head_index(size_t id, size_t gid, __global uint3 *qDetails, int n);
uint q_get_tail_index(size_t id, size_t gid, __global uint3 *qDetails, int n);
void q_set_head_index(uint index, size_t id, size_t gid, __global uint3 *qDetails, int n);
void q_set_tail_index(uint index, size_t id, size_t gid, __global uint3 *qDetails, int n);
void q_set_last_op(uint type, size_t id, size_t gid,__global uint3 *qDetails, int n);
bool q_last_op_is_read(size_t id, size_t gid, __global uint3 *qDetails, int n);
bool q_last_op_is_write(size_t id, size_t gid, __global uint3 *qDetails, int n);
bool q_is_empty(size_t id, size_t gid, __global uint3 *qDetails, int n);
bool q_is_full(size_t id, size_t gid,__global uint3 *qDetails, int n);
bool q_read(uint2 *result, size_t id, __global uint2 *q, __global uint3 *qDetails, int n);
bool q_write(uint2 value, size_t id, __global uint2 *q, __global uint3 *qDetails, int n);

/* Simple kernel to test queue functionality. */
__kernel void qtest(__global uint2 *q, __global uint3 *qDetails, int n, __global int *state) {
  size_t id = get_global_id(0);
  q_write(11, id, q, qDetails, n);
}

/* Returns the array index of the head element of the queue specifided by 'id' */
uint q_get_head_index(size_t id, size_t gid, __global uint3 *qDetails, int n) {
  return qDetails[id * n + gid].x;
}

/* Returns the array index of the tail element of the queue specified by 'id'. */
uint q_get_tail_index(size_t id, size_t gid,__global uint3 *qDetails, int n) {
  return qDetails[id * n + gid].y;
}

/* Set the array index of the head element of the queue specified by 'id'. */
void q_set_head_index(uint index, size_t id, size_t gid, __global uint3 *qDetails, int n) {
  qDetails[id * n + gid].x = index;
}

/* Set the array index of the tail element of the queue specified by 'id'. */
void q_set_tail_index(uint index, size_t id, size_t gid,__global uint3 *qDetails, int n) {
  qDetails[id * n + gid].y = index;
}

/* Set the type of the operation last performed on the queue specified by 'id'. */
void q_set_last_op(uint type, size_t id, size_t gid, __global uint3 *qDetails, int n) {
  qDetails[id * n + gid].z = type;
}

/* Returns true if the last operation performed on the queue specified by 'id' is a read, false otherwise. */
bool q_last_op_is_read(size_t id, size_t gid,__global uint3 *qDetails, int n) {
  return qDetails[id * n + gid].z == READ;
}

/* Returns true if the last operation performed on the queue specified by 'id' is a write, false otherwise. */
bool q_last_op_is_write(size_t id, size_t gid,__global uint3 *qDetails, int n) {
  return qDetails[id * n + gid].z != READ;
}

/* Returns true if the queue is empty, false otherwise. */
bool q_is_empty(size_t id, size_t gid,  __global uint3 *qDetails, int n) {
  return (q_get_head_index(id, gid, qDetails, n) == q_get_tail_index(id, gid, qDetails, n))
    && q_last_op_is_read(id, gid, qDetails, n);
}

/* Returns true if the queue is full, false otherwise. */
bool q_is_full(size_t id, size_t gid, __global uint3 *qDetails, int n) {
  return (q_get_head_index(id, gid, qDetails, n) == q_get_tail_index(id, gid, qDetails, n))
    && q_last_op_is_write(id, gid, qDetails, n);
}

/* Read the value located at the head index into 'result' from the queue specified by 'id'.
 * Returns true if succcessful (queue is not empty), false otherwise. */ 
bool q_read(uint2 *result, size_t id, __global uint2 *q, __global uint3 *qDetails, int n) {
  size_t gid = get_global_id(0);
  if (q_is_empty(id, gid, qDetails, n)) {
    return false;
  }

  int index = q_get_head_index(id, gid, qDetails, n);
  *result = q[(gid * n * QUEUE_SIZE) + (id * QUEUE_SIZE) + index];
  q_set_head_index((index + 1) % QUEUE_SIZE, id, gid, qDetails, n);
  q_set_last_op(READ, id, gid, qDetails, n);
  return true;
}

/* Write a value into the tail index of the queue specified by 'id'.
 * Returns true if successful (queue is not full), false otherwise. */
bool q_write(uint2 value, size_t id, __global uint2 *q, __global uint3 *qDetails, int n) {
  size_t gid = get_global_id(0);
  if (q_is_full(id, gid, qDetails, n)) {
    return false;
  }

  int index = q_get_tail_index(id, gid, qDetails, n);
  q[(gid * n * QUEUE_SIZE) + (id * QUEUE_SIZE) + index] = value;
  q_set_tail_index((index + 1) % QUEUE_SIZE, id, gid, qDetails, n);
  q_set_last_op(WRITE, id, gid, qDetails, n);
  return true;
}
