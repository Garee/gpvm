/* Macros */
#define READ 0
#define WRITE 1
#define COMPLETE -1
#define QUEUE_SIZE 16
#define SUBT_SIZE 256

/* Types */
#define __global
#define __kernel
#define uint unsigned int
#define uchar unsigned char
#define ushort unsigned short
#define ulong unsigned long
#define bool int
#define true 1
#define false 0

typedef struct uint2 {
  unsigned int x;
  unsigned int y;
} uint2;

typedef struct ushort2 {
  unsigned short x;
  unsigned short y;
} ushort2;

/* Functions */
#define get_global_id(n) 0

ushort2 as_ushort2(uint n) {
  ushort2 u;
  u.x = (n >> 16);
  u.y = (n & 0xFFFF);
  return u;
}

uint as_uint(ushort2 n) {
  uint u = n.x;
  u = (u << 16) | n.y;
  return u;
}
