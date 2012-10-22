uint q_get_head_index(size_t id, __global uint2 *queues,  int qsize) {
  return queues[id * qsize].x;
}

uint q_get_tail_index(size_t id, __global uint2 *queues,  int qsize) {
  return queues[id * qsize].y;  
}

void q_set_head_index(uint index, size_t id, __global uint2 *queues,  int qsize) {
  queues[id * qsize].x = index;
}

void q_set_tail_index(uint index, size_t id, __global uint2 *queues,  int qsize) {
  queues[id * qsize].y = index;
}

bool q_is_empty(size_t id, __global uint2 *queues,  int qsize) {
  return q_get_head_index(id, queues, qsize) == q_get_tail_index(id, queues, qsize);
}

bool q_is_full(size_t id, __global uint2 *queues,  int qsize) {
  return ((q_get_tail_index(id, queues, qsize) + 1) % qsize) == q_get_head_index(id, queues, qsize);
}

uint2 q_read(size_t id, __global uint2 *queues,  int qsize) {
  int index = q_get_head_index(id, queues, qsize);
  uint2 result = queues[(id * qsize) + index];
  q_set_head_index((index + 1) % qsize, id, queues, qsize);
  return result;
}

void q_write(uint2 value, size_t id, __global uint2 *queues,  int qsize) {
  int index = q_get_tail_index(id, queues, qsize);
  queues[(id * qsize) + index] = value;
  q_set_tail_index((index + 1) % qsize, id, queues, qsize);
}

__kernel void qtest(__global uint2 *queues,  int qsize) {
  size_t gid = get_global_id(0);
  for (int i = 1; i < 16; i++) {
    q_write(i * 10, gid, queues, qsize);
  }
  printf("%d\n", q_is_empty(gid, queues, qsize));
}
