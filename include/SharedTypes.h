#ifndef _SHAREDTYPES_H_
#define _SHAREDTYPES_H_

#include "SharedMacros.h"

/* Contains type definitions that are used by both the host and kernel programs. */

/* Replace the types for use in host program source. */
#ifdef _IN_HOST
#define uchar cl_uchar
#define ushort cl_ushort
#define uint cl_uint
#define uint2 cl_uint2
#define ulong cl_ulong
#endif

/* A packet is 2x32bit words. */
typedef uint2 packet;

/* Bytecode consists of 64bit words. */
typedef ulong bytecode;

/* A subtask table record. */
typedef struct subt_rec {
  uint service_id;            // [32bits] Opcode
  uint args[QUEUE_SIZE];      // [32bits] Pointers to data or constants.
  uchar arg_mode[QUEUE_SIZE]; // [4bits] Arg status, [4bits] Arg mode
  uchar subt_status;          // [4bits]  Subtask status, [4bits] number of args absent.
  uchar return_to;            // [8bits]
  ushort return_as;           // [16bits] Subtask address + argument position.
} subt_rec; 

/* The subtask table with associated available record stack. */
typedef struct subt {
  subt_rec recs[SUBT_SIZE];             // The subtask table records.
  ushort av_recs[SUBT_SIZE + 1];        // Stack of available records.
} subt;

#endif 
