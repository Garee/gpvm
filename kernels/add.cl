__kernel void add(__global *a, __global *b, __global *results) {
  size_t gid = get_global_id(0);
  results[gid] = a[gid] + b[gid];
}
