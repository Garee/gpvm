/* Macros */
#define READ 0
#define WRITE 1
#define COMPLETE -1
#define QUEUE_SIZE 16

/* Types */
#define __global
#define __kernel
#define uint unsigned int
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

ushort2 dummy_ushort2;

/* Functions */
#define get_global_id(n) 0
#define as_ushort2(n) dummy_ushort2
#define as_uint(n) 0
