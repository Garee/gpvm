#define READ 0
#define WRITE 1

/* Returns the array index of the head element of the queue specifided by 'id' */
uint q_get_head_index(size_t id, __global uint3 *queueDetails) {
  return queueDetails[id].x;
}

/* Returns the array index of the tail element of the queue specified by 'id'. */
uint q_get_tail_index(size_t id, __global uint3 *queueDetails) {
  return queueDetails[id].y;
}

/* Returns true if the last operation performed on the queue specified by 'id' is a read, false otherwise. */
bool q_last_op_is_read(size_t id, __global uint3 *queueDetails) {
  return queueDetails[id].z == READ;
}

/* Returns true if the last operation performed on the queue specified by 'id' is a write, false otherwise. */
bool q_last_op_is_write(size_t id, __global uint3 *queueDetails) {
  return queueDetails[id].z == WRITE;
}

/* Set the array index of the head element of the queue specified by 'id'. */
void q_set_head_index(uint index, size_t id, __global uint3 *queueDetails) {
  queueDetails[id].x = index;
}

/* Set the array index of the tail element of the queue specified by 'id'. */
void q_set_tail_index(uint index, size_t id, __global uint3 *queueDetails) {
  queueDetails[id].y = index;
}

/* Set the type of the operation last performed on the queue specified by 'id'. */
void q_set_last_op(uint type, size_t id, __global uint3 *queueDetails) {
  queueDetails[id].z = type;
}

/* Returns true if the queue is empty, false otherwise. */
bool q_is_empty(size_t id,  __global uint3 *queueDetails) {
  return (q_get_head_index(id, queueDetails) == q_get_tail_index(id, queueDetails)) && q_last_op_is_read(id, queueDetails);
}

/* Returns true if the queue is full, false otherwise. */
bool q_is_full(size_t id, __global uint3 *queueDetails) {
  return (q_get_head_index(id, queueDetails) == q_get_tail_index(id, queueDetails)) && q_last_op_is_write(id, queueDetails);
}

/* Read the value located at the head index into 'result' from the queue specified by 'id'.
 * Returns true if succcessful (queue is not empty), false otherwise. */ 
bool q_read(uint2 *result, size_t id, __global uint2 *queues,  int qsize, __global uint3 *queueDetails) {
  if (q_is_empty(id, queueDetails)) {
    return false;
  }

  int index = q_get_head_index(id, queueDetails);
  *result = queues[(id * qsize) + index];
  q_set_head_index((index + 1) % qsize, id, queueDetails);
  q_set_last_op(READ, id, queueDetails);
  return true;
}

/* Write a value into the tail index of the queue specified by 'id'.
 * Returns true if successful (queue is not full), false otherwise. */
bool q_write(uint2 value, size_t id, __global uint2 *queues,  int qsize, __global uint3 *queueDetails) {
  if (q_is_full(id, queueDetails)) {
    return false;
  }

  int index = q_get_tail_index(id, queueDetails);
  queues[(id * qsize) + index] = value;
  q_set_tail_index((index + 1) % qsize, id, queueDetails);
  q_set_last_op(WRITE, id, queueDetails);
  return true;
}

/* Simple kernel to test queue functionality. */
__kernel void qtest(__global uint2 *queues, int qsize, __global uint3 *queueDetails) {
  size_t gid = get_global_id(0);
  for (int i = 0; i < 16; i++) {
    q_write(i * 10, gid, queues, qsize, queueDetails);
  }
  printf("full %d\n", q_is_full(gid, queueDetails));
  printf("empty %d\n", q_is_empty(gid, queueDetails));
}
