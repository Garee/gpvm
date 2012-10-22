/* Returns the array index of the head element of the queue specifided by 'id' */
uint q_get_head_index(size_t id, __global uint2 *queues,  int qsize) {
  return queues[(id * qsize) + qsize - 1].x;
}

/* Returns the array index of the tail element of the queue specified by 'id'. */
uint q_get_tail_index(size_t id, __global uint2 *queues,  int qsize) {
  return queues[(id * qsize) + qsize - 1].y;  
}

/* Set the array index of the head element of the queue specified by 'id'. */
void q_set_head_index(uint index, size_t id, __global uint2 *queues,  int qsize) {
  queues[(id * qsize) + qsize - 1].x = index;
}

/* Set the array index of the tail element of the queue specified by 'id'. */
void q_set_tail_index(uint index, size_t id, __global uint2 *queues,  int qsize) {
  queues[(id * qsize) + qsize - 1].y = index;
}

/* Returns true if the queue is empty, false otherwise. */
bool q_is_empty(size_t id, __global uint2 *queues,  int qsize) {
  return q_get_head_index(id, queues, qsize) == q_get_tail_index(id, queues, qsize);
}

/* Returns true if the queue is full, false otherwise. */
bool q_is_full(size_t id, __global uint2 *queues,  int qsize) {
  return (q_get_tail_index(id, queues, qsize) + 1) % qsize == q_get_head_index(id, queues, qsize);
}

/* Read the value located at the head index into 'result' from the queue specified by 'id'.
 * Returns true if succcessful (queue is not empty), false otherwise. */ 
bool q_read(uint2 *result, size_t id, __global uint2 *queues,  int qsize) {
  if (q_is_empty(id, queues, qsize)) {
    return false;
  }

  int index = q_get_head_index(id, queues, qsize);
  *result = queues[(id * qsize) + index];
  q_set_head_index((index + 1) % qsize, id, queues, qsize);
  return true;
}

/* Write a value into the tail index of the queue specified by 'id'.
 * Returns true if successful (queue is not full), false otherwise. */
bool q_write(uint2 value, size_t id, __global uint2 *queues,  int qsize) {
  if (q_is_full(id, queues, qsize)) {
    return false;
  }

  int index = q_get_tail_index(id, queues, qsize);
  queues[(id * qsize) + index] = value;
  q_set_tail_index((index + 1) % qsize, id, queues, qsize);
  return true;
}

/* Simple kernel to test queue functionality. */
__kernel void qtest(__global uint2 *queues, int qsize) {
  size_t gid = get_global_id(0);
  switch (gid) {
  case 0:
    q_write(9, 1, queues, qsize);  
    break;
  case 1:
    q_write(5, 0, queues, qsize);  
    break;
  case 3:
    q_write(44, 1, queues, qsize);
    break;
  }
}
