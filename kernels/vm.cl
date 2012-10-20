__kernel void add(__global uint2 *a, __global uint2 *b) {
  size_t gid = get_global_id(0);
  b[gid] = a[gid] + 1;
}
