/* Used to create, manipulate and access packet information. */
#define PKT_TYPE_SHIFT 0
#define PKT_DEST_SHIFT 2
#define PKT_ARG_SHIFT  10
#define PKT_SUB_SHIFT  14
#define PKT_TYPE_MASK 0x3      // 00000000000000000000000000000011
#define PKT_DEST_MASK 0x3FC    // 00000000000000000000001111111100
#define PKT_ARG_MASK  0x3C00   // 00000000000000000011110000000000
#define PKT_SUB_MASK  0xFFC000 // 00000000111111111100000000000000

/* Packet Types */
#define ERROR     0
#define REFERENCE 1
#define DATA      2
#define REQUEST   3

typedef uint2 packet;

uint pkt_get_type(packet p);
uint pkt_get_dest(packet p);
uint pkt_get_arg(packet p);
uint pkt_get_sub(packet p);
uint pkt_get_payload(packet p);
void pkt_set_type(packet *p, uint type);
void pkt_set_dest(packet *p, uint dest);
void pkt_set_arg(packet *p, uint arg);
void pkt_set_sub(packet *p, uint sub);
void pkt_set_payload(packet *p, uint payload);
packet pkt_create(uint type, uint dest, uint arg, uint sub, uint payload);

void transferRQ(__global uint2 *rq,  __global uint2 *q, int n);
uint q_get_head_index(size_t id, size_t gid, __global uint2 *q, int n);
uint q_get_tail_index(size_t id, size_t gid, __global uint2 *q, int n);
void q_set_head_index(uint index, size_t id, size_t gid, __global uint2 *q, int n);
void q_set_tail_index(uint index, size_t id, size_t gid, __global uint2 *q, int n);
void q_set_last_op(uint type, size_t id, size_t gid,__global uint2 *q, int n);
bool q_last_op_is_read(size_t id, size_t gid, __global uint2 *q, int n);
bool q_last_op_is_write(size_t id, size_t gid, __global uint2 *q, int n);
bool q_is_empty(size_t id, size_t gid, __global uint2 *q, int n);
bool q_is_full(size_t id, size_t gid,__global uint2 *q, int n);
bool q_read(uint2 *result, size_t id, __global uint2 *q, int n);
bool q_write(uint2 value, size_t id, __global uint2 *q, int n);

/**************************/
/******* The Kernel *******/
/**************************/
__kernel void vm(__global uint2 *q,
		 __global uint2 *rq,
		 int n, 
		 __global int *state) 
{
  size_t gid = get_global_id(0);

  packet p = pkt_create(ERROR, 7, 0, 0, 0);
  if (*state == WRITE) {
    transferRQ(rq, q, n);
  } else {
    q_write(p, gid, q, n);
  }

  *state = COMPLETE;
}

/**************************/
/**** Packet Functions ****/
/**************************/

uint pkt_get_type(packet p) {
  return (p.x & PKT_TYPE_MASK) >> PKT_TYPE_SHIFT;
}

uint pkt_get_dest(packet p) {
  return (p.x & PKT_DEST_MASK) >> PKT_DEST_SHIFT;
}

uint pkt_get_arg(packet p) {
  return (p.x & PKT_ARG_MASK) >> PKT_ARG_SHIFT;
}

uint pkt_get_sub(packet p) {
  return (p.x & PKT_SUB_MASK) >> PKT_SUB_SHIFT;
}

uint pkt_get_payload(packet p) {
  return p.y;
}

void pkt_set_type(packet *p, uint type) {
  (*p).x = ((*p).x & ~PKT_TYPE_MASK) | ((type << PKT_TYPE_SHIFT) & PKT_TYPE_MASK);
}

void pkt_set_dest(packet *p, uint dest) {
  (*p).x = ((*p).x & ~PKT_DEST_MASK) | ((dest << PKT_DEST_SHIFT) & PKT_DEST_MASK);
}

void pkt_set_arg(packet *p, uint arg) {
  (*p).x = ((*p).x & ~PKT_ARG_MASK) | ((arg << PKT_ARG_SHIFT) & PKT_ARG_MASK);
}

void pkt_set_sub(packet *p, uint sub) {
  (*p).x = ((*p).x & ~PKT_SUB_MASK) | ((sub << PKT_SUB_SHIFT) & PKT_SUB_MASK);
}

void pkt_set_payload(packet *p, uint payload) {
  (*p).y = payload;
}

packet pkt_create(uint type, uint dest, uint arg, uint sub, uint payload) {
  packet p = 0;
  pkt_set_type(&p, type);
  pkt_set_dest(&p, dest);
  pkt_set_arg(&p, arg);
  pkt_set_sub(&p, sub);
  pkt_set_payload(&p, payload);
  return p;
}


/*************************/
/**** Queue Functions ****/
/*************************/

/* Returns the array index of the head element of the queue specifided by 'id' */
uint q_get_head_index(size_t id, size_t gid, __global uint2 *q, int n) {
  ushort2 indices = q[id * n + gid].x;
  return indices.x;
}

/* Returns the array index of the tail element of the queue specified by 'id'. */
uint q_get_tail_index(size_t id, size_t gid,__global uint2 *q, int n) {
  ushort2 indices = q[id * n + gid].x;
  return indices.y;
}

/* Set the array index of the head element of the queue specified by 'id'. */
void q_set_head_index(uint index, size_t id, size_t gid, __global uint2 *q, int n) {
  ushort2 indices = q[id * n + gid].x;
  indices.x = index;
  q[id * n + gid].x = as_uint(indices);
}

/* Set the array index of the tail element of the queue specified by 'id'. */
void q_set_tail_index(uint index, size_t id, size_t gid,__global uint2 *q, int n) {
  ushort2 indices = q[id * n + gid].x;
  indices.y = index;
  q[id * n + gid].x = as_uint(indices);
}

/* Set the type of the operation last performed on the queue specified by 'id'. */
void q_set_last_op(uint type, size_t id, size_t gid, __global uint2 *q, int n) {
  q[id * n + gid].y = type;
}

/* Returns true if the last operation performed on the queue specified by 'id' is a read, false otherwise. */
bool q_last_op_is_read(size_t id, size_t gid,__global uint2 *q, int n) {
  return q[id * n + gid].y == READ;
}

/* Returns true if the last operation performed on the queue specified by 'id' is a write, false otherwise. */
bool q_last_op_is_write(size_t id, size_t gid,__global uint2 *q, int n) {
  return q[id * n + gid].y != READ;
}

/* Returns true if the queue is empty, false otherwise. */
bool q_is_empty(size_t id, size_t gid,  __global uint2 *q, int n) {
  return (q_get_head_index(id, gid, q, n) == q_get_tail_index(id, gid, q, n))
    && q_last_op_is_read(id, gid, q, n);
}

/* Returns true if the queue is full, false otherwise. */
bool q_is_full(size_t id, size_t gid, __global uint2 *q, int n) {
  return (q_get_head_index(id, gid, q, n) == q_get_tail_index(id, gid, q, n))
    && q_last_op_is_write(id, gid, q, n);
}

/* Read the value located at the head index into 'result' from the queue specified by 'id'.
 * Returns true if succcessful (queue is not empty), false otherwise. */ 
bool q_read(uint2 *result, size_t id, __global uint2 *q, int n) {
  size_t gid = get_global_id(0);
  if (q_is_empty(id, gid, q, n)) {
    return false;
  }

  int index = q_get_head_index(id, gid, q, n);
  *result = q[(n*n) + (gid * n * QUEUE_SIZE) + (id * QUEUE_SIZE) + index];
  q_set_head_index((index + 1) % QUEUE_SIZE, id, gid, q, n);
  q_set_last_op(READ, id, gid, q, n);
  return true;
}

/* Write a value into the tail index of the queue specified by 'id'.
 * Returns true if successful (queue is not full), false otherwise. */
bool q_write(uint2 value, size_t id, __global uint2 *q, int n) {
  size_t gid = get_global_id(0);
  if (q_is_full(id, gid, q, n)) {
    return false;
  }

  int index = q_get_tail_index(id, gid, q, n);
  q[(n*n) + (gid * n * QUEUE_SIZE) + (id * QUEUE_SIZE) + index] = value;
  q_set_tail_index((index + 1) % QUEUE_SIZE, id, gid, q, n);
  q_set_last_op(WRITE, id, gid, q, n);
  return true;
}

void transferRQ(__global uint2 *rq,  __global uint2 *q, int n) {
  size_t gid = get_global_id(0);
  uint2 packet;
  for (int i = 0; i < n; i++) {
    while (!q_is_empty(i, gid, rq, n)) {
      q_read(&packet, i, rq, n);
      q_write(packet, i, q, n);
    }
  }
}
