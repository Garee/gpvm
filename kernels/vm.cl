__kernel void add(__global uint2 *queues) {
  size_t gid = get_global_id(0);
  queues[gid].x = 1;
}
