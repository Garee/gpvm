#ifdef KERNEL_TEST_ENABLED
#include "../tests/kerneltypes.h"
#endif

#include "SharedMacros.h"
#include "SharedTypes.h"

/* Base K_B symbol. */
#define SYMBOL_KB_ZERO 0x6040000000000000

/* Base K_P symbol. */
#define SYMBOL_KP_ZERO 0x8000000000000000

/* Used to access the information stored within a subtask record. */
#define SUBTREC_NARGS_ABSENT_MASK    0xF  // 00001111
#define SUBTREC_NARGS_ABSENT_SHIFT   0

#define SUBTREC_STATUS_MASK          0xF0 // 11110000
#define SUBTREC_STATUS_SHIFT         4

#define SUBTREC_RETURN_AS_ADDR_MASK  0xFF00 // 1111111100000000
#define SUBTREC_RETURN_AS_ADDR_SHIFT 8

#define SUBTREC_RETURN_AS_POS_MASK   0xFF   // 0000000011111111
#define SUBTREC_RETURN_AS_POS_SHIFT  0

/* Subtask record status. */
#define NEW        0
#define PROCESSING 1
#define PENDING    2

/* Subtask record arg status. */
#define ABSENT     0
#define REQUESTING 1
#define PRESENT    2

/* Used to access symbol information. */

// 4  :3         :1    :2       :6       :16 2|4                         :32 (8|8|8|8)
// K_S:(Datatype):(Ext):(Quoted):Task    :Mode|Reg|2Bits|NArgs           :SCLId|SCId|Opcode
// K_R:(Datatype):(Ext):Quoted  :CodePage:6Bits|5Bits|CodeAddress        :Name
// K_B:Datatype  :0    :Quoted  :Task    :16Bits                         :Value

#define SYMBOL_KIND_MASK     0xF000000000000000UL // 11110000000000000000000000000000 00000000000000000000000000000000
#define SYMBOL_KIND_SHIFT    60

#define SYMBOL_QUOTED_MASK   0xc0000000000000UL   // 00000000110000000000000000000000 00000000000000000000000000000000
#define SYMBOL_QUOTED_SHIFT  54

#define SYMBOL_SUBTASK_MASK  0xFFFF00000000UL     // 00000000000000001111111111111111 00000000000000000000000000000000
#define SYMBOL_SUBTASK_SHIFT 32

#define SYMBOL_NARGS_MASK    0xF00000000UL        // 00000000000000000000000000001111 00000000000000000000000000000000
#define SYMBOL_NARGS_SHIFT   32

#define SYMBOL_SNId_MASK     0xFF000000UL         // 00000000000000000000000000000000 11111111000000000000000000000000
#define SYMBOL_SNId_SHIFT    24

#define SYMBOL_OPCODE_MASK   0xFFFFFFFFUL         // 00000000000000000000000000000000 11111111111111111111111111111111
#define SYMBOL_OPCODE_SHIFT  0

#define SYMBOL_VALUE_MASK    0xFFFFFFFFUL         // 00000000000000000000000000000000 11111111111111111111111111111111
#define SYMBOL_VALUE_SHIFT   0

/* Definition of symbol kinds. */
#define K_S 0 // Contains information needed to create subtask record.
#define K_R 4 // Reference symbol.
#define K_B 6 // Data symbol.
#define K_P 8 // Contains a pointer to scratch memory.

/* Definition of computation class types - Placeholders */
#define ALU 0

/* ALU methods */
#define ADD 0
#define SUB 1

/***********************************/
/******* Function Prototypes *******/
/***********************************/
void parse_pkt(packet p, __global uint2 *q, int n, __global bytecode *cStore, __global subt *subt, __global char *scratch);
uint parse_subtask(uint source,
                   uint arg_pos,
                   uint subtask,
                   uint address,
                   __global uint2 *q,
                   int n,
                   __global bytecode *cStore,
                   __global subt *subt,
                   __global char *scratch);
bytecode service_compute(__global subt* subt, uint subtask,__global char *scratch);
bool computation_complete(__global packet *q, int n);
uint get_arg_value(uint arg_pos, __global subt_rec *rec, __global char *scratch);

void transferRQ(__global uint2 *rq,  __global uint2 *q, int n);

void subt_store_payload(uint payload, uint payload_type, uint arg_pos, ushort i, __global subt *subt);
bool subt_is_ready(ushort i, __global subt *subt);
__global subt_rec *subt_get_rec(ushort i, __global subt *subt);
bool subt_push(ushort i, __global subt *subt);
bool subt_pop(ushort *result, __global subt *subt);
bool subt_is_full(__global subt *subt);
bool subt_is_empty(__global subt *subt);
void subt_cleanup(ushort i, __global subt *subt);
ushort subt_top(__global subt *subt);
void subt_set_top(__global subt *subt, ushort i);

uint subt_rec_get_service_id(__global subt_rec *r);
bytecode subt_rec_get_arg(__global subt_rec *r, uint arg_pos);
uint subt_rec_get_arg_status(__global subt_rec *r, uint arg_pos);
uint subt_rec_get_subt_status(__global subt_rec *r);
uint subt_rec_get_nargs_absent(__global subt_rec *r);
uint subt_rec_get_return_to(__global subt_rec *r);
uint subt_rec_get_return_as(__global subt_rec *r);
uint subt_rec_get_return_as_addr(__global subt_rec *r);
uint subt_rec_get_return_as_pos(__global subt_rec *r);
void subt_rec_set_service_id(__global subt_rec *r, uint service_id);
void subt_rec_set_arg(__global subt_rec *r, uint arg_pos, bytecode arg);
void subt_rec_set_arg_status(__global subt_rec *r, uint arg_pos, uint status);
void subt_rec_set_subt_status(__global subt_rec *r, uint status);
void subt_rec_set_nargs_absent(__global subt_rec *r, uint n);
void subt_rec_set_return_to(__global subt_rec *r, uint return_to);
void subt_rec_set_return_as(__global subt_rec *r, uint return_as);
void subt_rec_set_return_as_addr(__global subt_rec *r, uint return_as_addr);
void subt_rec_set_return_as_pos(__global subt_rec *r, uint return_as_pos);

bytecode symbol_KS_create(uint nargs, uint opcode);
bytecode symbol_KR_create(uint subtask, uint SNId);
bytecode symbol_KB_create(uint value);
uint symbol_get_kind(bytecode s);
bool symbol_is_quoted(bytecode s);
uint symbol_get_opcode(bytecode s);
uint symbol_get_SNId(bytecode s);
uint symbol_get_subtask(bytecode s);
uint symbol_get_nargs(bytecode s);
uint symbol_get_value(bytecode s);
void symbol_set_kind(bytecode *s, ulong kind);
void symbol_quote(bytecode *s);
void symbol_unquote(bytecode *s);
void symbol_set_opcode(bytecode *s, ulong opcode);
void symbol_set_SNId(bytecode *s, ulong SNId);
void symbol_set_subtask(bytecode *s, ulong subtask);
void symbol_set_nargs(bytecode *s, ulong nargs);
void symbol_set_value(bytecode *s, ulong value);

uint q_get_head_index(size_t id, size_t gid, __global packet *q, int n);
uint q_get_tail_index(size_t id, size_t gid, __global packet *q, int n);
void q_set_head_index(uint index, size_t id, size_t gid, __global packet *q, int n);
void q_set_tail_index(uint index, size_t id, size_t gid, __global packet *q, int n);
void q_set_last_op(uint type, size_t id, size_t gid,__global packet *q, int n);
bool q_last_op_is_read(size_t id, size_t gid, __global packet *q, int n);
bool q_last_op_is_write(size_t id, size_t gid, __global packet *q, int n);
bool q_is_empty(size_t id, size_t gid, __global packet *q, int n);
bool q_is_full(size_t id, size_t gid,__global packet *q, int n);
uint q_size(size_t id, size_t gid, __global packet *q, int n);
bool q_read(packet *result, size_t id, __global packet *q, int n);
bool q_write(packet value, size_t id, __global packet *q, int n);

packet pkt_create(uint type, uint source, uint arg, uint sub, uint payload);
uint pkt_get_type(packet p);
uint pkt_get_source(packet p);
uint pkt_get_arg_pos(packet p);
uint pkt_get_sub(packet p);
uint pkt_get_payload_type(packet p);
uint pkt_get_payload(packet p);
void pkt_set_type(packet *p, uint type);
void pkt_set_source(packet *p, uint source);
void pkt_set_arg_pos(packet *p, uint arg);
void pkt_set_sub(packet *p, uint sub);
void pkt_set_payload_type(packet *p, uint ptype);
void pkt_set_payload(packet *p, uint payload);

/**************************/
/******* The Kernel *******/
/**************************/
__kernel void vm(__global packet *q,            /* Compute unit queues. */
                 __global packet *rq,           /* Transfer queues for READ state. */
                 int n,                         /* The number of compute units. */
                 __global int *state,           /* Are we in the READ or WRITE state? */
                 __global bytecode *cStore,     /* The code store. */
                 __global subt *subt,           /* The subtask table. */
                 __global char *scratch         /* Scratch memory for temporary results. */
                 ) {
  if (*state == WRITE) {
    transferRQ(rq, q, n);
    if (computation_complete(q, n)) {
      *state = COMPLETE;
    }
  } else {
    for (int i = 0; i < n; i++) {
      packet p;
      while (q_read(&p, i, q, n)) {
        parse_pkt(p, rq, n, cStore, subt, scratch);
      }
    }
  }
}

/* Is the entire computation complete? When all compute units are inactive (no packets in their queues)
   the computation is complete. */
bool computation_complete(__global packet *q, int n) {
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      if (!q_is_empty(i, j, q, n)) {
        return false;
      }
    }
  }
  
  return true;
}

void parse_pkt(packet p, __global uint2 *q, int n, __global bytecode *cStore, __global subt *subt, __global char *scratch) {
  uint type = pkt_get_type(p);
  uint source = pkt_get_source(p);
  uint arg_pos = pkt_get_arg_pos(p);
  uint subtask = pkt_get_sub(p);
  uint payload_type = pkt_get_payload_type(p);
  uint address = pkt_get_payload(p);

  switch (type) {
  case ERROR:
    break;
    
  case REFERENCE: {
    /* Create a new subtask record */
    uint ref_subtask = parse_subtask(source, arg_pos, subtask, address, q, n, cStore, subt, scratch);

    if (subt_is_ready(ref_subtask, subt)) {
      /* Perform the computation. */
      bytecode result = service_compute(subt, ref_subtask, scratch);

      /* Create a new packet containing the computation results. */
      packet p = pkt_create(DATA, get_global_id(0), arg_pos, subtask, symbol_get_value(result));
      
      if (symbol_get_kind(result) == K_P) {
        pkt_set_payload_type(&p, PTR);
      } else {
        pkt_set_payload_type(&p, VAL);
      }

      /* Send the result back to the compute unit that sent the reference request. */
      q_write(p, source, q, n);

      /* Free up the subtask record so that it may be re-used. */
      subt_cleanup(ref_subtask, subt);
    }
    break;
  }
    
  case DATA:
    /* Store the data in the subtask record. */
    subt_store_payload(address, payload_type, arg_pos, subtask, subt);

    if (subt_is_ready(subtask, subt)) {
      /* Perform the computation. */
      bytecode result = service_compute(subt, subtask, scratch);

      /* Figure out where to send the result to. */
      __global subt_rec *rec = subt_get_rec(subtask, subt);
      uint return_to = subt_rec_get_return_to(rec);
      uint return_as_addr = subt_rec_get_return_as_addr(rec);
      uint return_as_pos = subt_rec_get_return_as_pos(rec);
      
      /* Create and send new packet containing the computation results. */
      packet p = pkt_create(DATA, get_global_id(0), return_as_pos, return_as_addr, symbol_get_value(result));
      
      if (symbol_get_kind(result) == K_P) {
        pkt_set_payload_type(&p, PTR);
      } else {
        pkt_set_payload_type(&p, VAL);
      }

      q_write(p, return_to, q, n);
      
      /* Free up the subtask record so that it may be re-used. */
      subt_cleanup(subtask, subt);
    }
    break;
  }
}

uint parse_subtask(uint source,
                   uint arg_pos,
                   uint subtask,
                   uint address,
                   __global uint2 *q,
                   int n,
                   __global bytecode *cStore,
                   __global subt *subt,
                   __global char *scratch
                   ) {
  /* Get an available subtask record from the stack */
  ushort av_index;
  while (!subt_pop(&av_index, subt)) {}
  __global subt_rec *rec = subt_get_rec(av_index, subt);

  /* Get the K_S symbol from the code store. */
  bytecode symbol = cStore[address * QUEUE_SIZE];
  
  /* Create a new subtask record. */
  uint nargs = symbol_get_nargs(symbol);
  uint opcode = symbol_get_opcode(symbol);
  subt_rec_set_subt_status(rec, NEW);
  subt_rec_set_nargs_absent(rec, nargs);
  subt_rec_set_service_id(rec, opcode);
  subt_rec_set_return_to(rec, source);
  subt_rec_set_return_as_addr(rec, subtask);
  subt_rec_set_return_as_pos(rec, arg_pos);
  
  /* Begin argument processing */
  subt_rec_set_subt_status(rec, PROCESSING);

  for (int arg_pos = 1; arg_pos < nargs; arg_pos++) {
    /* Mark argument as absent. */
    subt_rec_set_arg_status(rec, arg_pos, ABSENT);
    
    /* Get the next symbol (K_R or K_B) */
    symbol = cStore[(address * QUEUE_SIZE) + arg_pos];
    
    switch (symbol_get_kind(symbol)) {
    case K_R:
      if (symbol_is_quoted(symbol)) {
        subt_store_payload(symbol, VAL, arg_pos, av_index, subt);
      } else {
        /* Create a packet to request a computation. */
        packet p = pkt_create(REFERENCE, get_global_id(0), arg_pos, av_index, symbol);

        /* Find out which service should perform the computation. */
        uint destination = symbol_get_SNId(symbol); // TODO run-time dest allocation
	
        /* Send the packet and request the computation. */
        q_write(p, destination, q, n);
	
        /* Mark argument as requested. */
        subt_rec_set_arg_status(rec, arg_pos, REQUESTING);
      }
      break;
      
    case K_B:
      subt_store_payload(symbol_get_value(symbol), VAL, arg_pos, av_index, subt);
      break;
    }
  }

  return av_index;
}

bytecode service_compute(__global subt* subt, uint subtask, __global char *scratch) {
  /* Get the opcode */
  __global subt_rec *rec = subt_get_rec(subtask, subt);
  uint opcode = subt_rec_get_service_id(rec);

  uint library = opcode >> 24; // SCLId
  uint class = (opcode & 0xFF0000) >> 16; // SCId
  uint method = opcode & 0xFFFF; // Method is last 16 bits?

  uint result = 0;
  switch (class) {
  case ALU:
    switch (method) {
    case ADD: {
      uint arg1 = get_arg_value(0, rec, scratch);
      uint arg2 = get_arg_value(1, rec, scratch);
      result = arg1 + arg2;
      break;
    }
      
    case SUB: {
      uint arg1 = get_arg_value(0, rec, scratch);
      uint arg2 = get_arg_value(1, rec, scratch);
      result = arg1 - arg2;
      break;
    }

    }
    break;
  }
  
  // If result written to scratch, return K_P symbol otherwise K_B.
  return SYMBOL_KB_ZERO + result;
}

uint get_arg_value(uint arg_pos, __global subt_rec *rec, __global char *scratch) {
  bytecode symbol = subt_rec_get_arg(rec, arg_pos);
  uint value = symbol_get_value(symbol);
  
  if (symbol_get_kind(symbol) == K_P) { // It's a pointer.
    // return scratch[get_global_id(0) * SCRATCH_SIZE + value];
  }

  /* It's a constant. */
  return value;
}

/*********************************/
/**** Subtask Table Functions ****/
/*********************************/

void subt_store_payload(uint payload, uint payload_type, uint arg_pos, ushort i, __global subt *subt) {
  __global subt_rec *rec = subt_get_rec(i, subt);

  if (payload_type == PTR) {
    subt_rec_set_arg(rec, arg_pos, SYMBOL_KP_ZERO + payload);
  } else {
    subt_rec_set_arg(rec, arg_pos, SYMBOL_KB_ZERO + payload);
  }

  uint nargs_absent = subt_rec_get_nargs_absent(rec) - 1;
  subt_rec_set_arg_status(rec, arg_pos, PRESENT);
  subt_rec_set_nargs_absent(rec, nargs_absent);
}

/* Is the subtask record at index i ready for computation? */
bool subt_is_ready(ushort i, __global subt *subt) {
  __global subt_rec *rec = subt_get_rec(i, subt);
  return subt_rec_get_nargs_absent(rec) == 0;
}

/* Return the subtask record at index i in the subtask table. */
__global subt_rec *subt_get_rec(ushort i, __global subt *subt) {
  return &(subt->recs[i]);
}

/* Remove the subtask record at index i from the subtask table and return
   it to the stack of available records. */
bool subt_push(ushort i, __global subt *subt) {
  if (subt_is_empty(subt)) {
    return false;
  }
  
  ushort top = subt_top(subt);
  subt->av_recs[top - 1] = i;
  subt_set_top(subt, top - 1);
  return true;
}

/* Return an available subtask record index from the subtask table. */
bool subt_pop(ushort *av_index, __global subt *subt) {
  if (subt_is_full(subt)) {
    return false;
  }

  ushort top = subt_top(subt);
  *av_index = subt->av_recs[top];
  subt_set_top(subt, top + 1);
  return true;
}

/* Remove and cleanup the subtask record at index i from the subtask table. */
void subt_cleanup(ushort i, __global subt *subt) {
  subt_push(i, subt);
}

/* Is the subtask table full? */
bool subt_is_full(__global subt *subt) {
  return subt_top(subt) == SUBT_SIZE + 1;
}

/* Is the subtask table empty? */
bool subt_is_empty(__global subt *subt) {
  return subt_top(subt) == 1;
}

/* Return the top of available records stack index. */
ushort subt_top(__global subt *subt) {
  return subt->av_recs[0];
}

/* Set the top of available records stack index. */
void subt_set_top(__global subt *subt, ushort i) {
  subt->av_recs[0] = i;
}

/**********************************/
/**** Subtask Record Functions ****/
/**********************************/
uint subt_rec_get_service_id(__global subt_rec *r) {
  return r->service_id;
}

bytecode subt_rec_get_arg(__global subt_rec *r, uint arg_pos) {
  return r->args[arg_pos];
}

uint subt_rec_get_arg_status(__global subt_rec *r, uint arg_pos) {
  return r->arg_status[arg_pos];
}

uint subt_rec_get_subt_status(__global subt_rec *r) {
  return (r->subt_status & SUBTREC_STATUS_MASK) >> SUBTREC_STATUS_SHIFT;
}

uint subt_rec_get_nargs_absent(__global subt_rec *r) {
  return (r->subt_status & SUBTREC_NARGS_ABSENT_MASK);
}

uint subt_rec_get_return_to(__global subt_rec *r) {
  return r->return_to;
}

uint subt_rec_get_return_as(__global subt_rec *r) {
  return r->return_as;
}

uint subt_rec_get_return_as_addr(__global subt_rec *r) {
  return r->return_as & SUBTREC_RETURN_AS_ADDR_MASK;
}

uint subt_rec_get_return_as_pos(__global subt_rec *r) {
  return r->return_as & SUBTREC_RETURN_AS_POS_MASK;
}

void subt_rec_set_service_id(__global subt_rec *r, uint service_id) {
  r->service_id = service_id;
}

void subt_rec_set_arg(__global subt_rec *r, uint arg_pos, bytecode arg) {
  r->args[arg_pos] = arg;
}

void subt_rec_set_arg_status(__global subt_rec *r, uint arg_pos, uint status) {
  r->arg_status[arg_pos] = status;
}

void subt_rec_set_subt_status(__global subt_rec *r, uint status) {
  r->subt_status = (r->subt_status & ~SUBTREC_STATUS_MASK)
    | ((status << SUBTREC_STATUS_SHIFT) & SUBTREC_STATUS_MASK);
}

void subt_rec_set_nargs_absent(__global subt_rec *r, uint n) {
  r->subt_status = (r->subt_status & ~SUBTREC_NARGS_ABSENT_MASK)
    | ((n << SUBTREC_NARGS_ABSENT_SHIFT) & SUBTREC_NARGS_ABSENT_MASK);
}

void subt_rec_set_return_to(__global subt_rec *r, uint return_to) {
  r->return_to = return_to;
}

void subt_rec_set_return_as(__global subt_rec *r, uint return_as) {
  r->return_as = return_as;
}

void subt_rec_set_return_as_addr(__global subt_rec *r, uint return_as_addr) {
  r->return_as = (r->return_as & ~SUBTREC_RETURN_AS_ADDR_MASK)
    | ((return_as_addr << SUBTREC_RETURN_AS_ADDR_SHIFT) & SUBTREC_RETURN_AS_ADDR_MASK);
}

void subt_rec_set_return_as_pos(__global subt_rec *r, uint return_as_pos) {
  r->return_as = (r->return_as & ~SUBTREC_RETURN_AS_POS_MASK)
    | ((return_as_pos << SUBTREC_RETURN_AS_POS_SHIFT) & SUBTREC_RETURN_AS_POS_MASK);
}

/**************************/
/**** Symbol Functions ****/
/**************************/

bytecode symbol_KS_create(uint nargs, uint opcode) {
  bytecode s = 0;
  symbol_set_kind(&s, K_S);
  symbol_unquote(&s);
  symbol_set_nargs(&s, nargs);
  symbol_set_opcode(&s, opcode);
  return s;
}

bytecode symbol_KR_create(uint subtask, uint SNId) {
  bytecode s = 0;
  symbol_set_kind(&s, K_R);
  symbol_unquote(&s);
  symbol_set_subtask(&s, subtask);
  symbol_set_SNId(&s, SNId);
  return s;
}

bytecode symbol_KB_create(uint value) {
  bytecode s = 0;
  symbol_set_kind(&s, K_B);
  symbol_unquote(&s);
  symbol_set_value(&s, value);
  return s;
}

/* Return the symbol kind. */
uint symbol_get_kind(bytecode s) {
  return (s & SYMBOL_KIND_MASK) >> SYMBOL_KIND_SHIFT;
}

/* Is the symbol quoted? */
bool symbol_is_quoted(bytecode s) {
  return (s & SYMBOL_QUOTED_MASK) >> SYMBOL_QUOTED_SHIFT;
}

/* Return the symbol (K_S) opcode. */
uint symbol_get_opcode(bytecode s) {
  return (s & SYMBOL_OPCODE_MASK) >> SYMBOL_OPCODE_SHIFT;
}

/* Return the symbol (K_R) SNId. */
uint symbol_get_SNId(bytecode s) {
  return (s & SYMBOL_SNId_MASK) >> SYMBOL_SNId_SHIFT;
}

/* Return the symbol (K_R) subtask. */
uint symbol_get_subtask(bytecode s) {
  return (s & SYMBOL_SUBTASK_MASK) >> SYMBOL_SUBTASK_SHIFT;
}

/* Return the symbol (K_S) nargs */
uint symbol_get_nargs(bytecode s) {
  return (s & SYMBOL_NARGS_MASK) >> SYMBOL_NARGS_SHIFT;
}

/* Return the symbol (K_B) value. */
uint symbol_get_value(bytecode s) {
  return (s & SYMBOL_VALUE_MASK) >> SYMBOL_VALUE_SHIFT;
}

void symbol_set_kind(bytecode *s, ulong kind) {
  *s = ((*s) & ~SYMBOL_KIND_MASK) | ((kind << SYMBOL_KIND_SHIFT) & SYMBOL_KIND_MASK);
}

void symbol_quote(bytecode *s) {
  *s = ((*s) & ~SYMBOL_QUOTED_MASK) | ((1UL << SYMBOL_QUOTED_SHIFT) & SYMBOL_QUOTED_MASK);
}

void symbol_unquote(bytecode *s) {
  *s = ((*s) & ~SYMBOL_QUOTED_MASK) | ((0UL << SYMBOL_QUOTED_SHIFT) & SYMBOL_QUOTED_MASK);
}

void symbol_set_opcode(bytecode *s, ulong opcode) {
  *s = ((*s) & ~SYMBOL_OPCODE_MASK) | ((opcode << SYMBOL_OPCODE_SHIFT) & SYMBOL_OPCODE_MASK);
}

void symbol_set_SNId(bytecode *s, ulong SNId) {
  *s = ((*s) & ~SYMBOL_SNId_MASK) | ((SNId << SYMBOL_SNId_SHIFT) & SYMBOL_SNId_MASK);
}

void symbol_set_subtask(bytecode *s, ulong subtask) {
  *s = ((*s) & ~SYMBOL_SUBTASK_MASK) | ((subtask << SYMBOL_SUBTASK_SHIFT) & SYMBOL_SUBTASK_MASK);
}

void symbol_set_nargs(bytecode *s, ulong nargs) {
  *s = ((*s) & ~SYMBOL_NARGS_MASK) | ((nargs << SYMBOL_NARGS_SHIFT) & SYMBOL_NARGS_MASK);
}

void symbol_set_value(bytecode *s, ulong value) {
  *s = ((*s) & ~SYMBOL_VALUE_MASK) | ((value << SYMBOL_VALUE_SHIFT) & SYMBOL_VALUE_MASK);
}

/*************************/
/**** Queue Functions ****/
/*************************/

/* Copy all compute unit owned queue values from the readQueue into the real queues. */
void transferRQ(__global uint2 *rq, __global uint2 *q, int n) {
  uint2 packet;
  for (int i = 0; i < n; i++) {
    while (q_read(&packet, i, rq, n)) {
      q_write(packet, i, q, n);
    }
  }
}

/* Returns the array index of the head element of the queue. */
uint q_get_head_index(size_t id, size_t gid, __global uint2 *q, int n) {
  ushort2 indices = as_ushort2(q[id * n + gid].x);
  return indices.x;
}

/* Returns the array index of the tail element of the queue. */
uint q_get_tail_index(size_t id, size_t gid,__global uint2 *q, int n) {
  ushort2 indices = as_ushort2(q[id * n + gid].x);
  return indices.y;
}

/* Set the array index of the head element of the queue. */
void q_set_head_index(uint index, size_t id, size_t gid, __global uint2 *q, int n) {
  ushort2 indices = as_ushort2(q[id * n + gid].x);
  indices.x = index;
  q[id * n + gid].x = as_uint(indices);
}

/* Set the array index of the tail element of the queue. */
void q_set_tail_index(uint index, size_t id, size_t gid,__global uint2 *q, int n) {
  ushort2 indices = as_ushort2(q[id * n + gid].x);
  indices.y = index;
  q[id * n + gid].x = as_uint(indices);
}

/* Set the type of the operation last performed on the queue. */
void q_set_last_op(uint type, size_t id, size_t gid, __global uint2 *q, int n) {
  q[id * n + gid].y = type;
}

/* Returns true if the last operation performed on the queue is a read, false otherwise. */
bool q_last_op_is_read(size_t id, size_t gid,__global uint2 *q, int n) {
  return q[id * n + gid].y == READ;
}

/* Returns true if the last operation performed on the queue is a write, false otherwise. */
bool q_last_op_is_write(size_t id, size_t gid,__global uint2 *q, int n) {
  return q[id * n + gid].y == WRITE;
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

/* Return the size of the queue. */
uint q_size(size_t id, size_t gid, __global uint2 *q, int n) {
  if (q_is_full(id, gid, q, n)) return QUEUE_SIZE;
  if (q_is_empty(id, gid, q, n)) return 0;
  uint head = q_get_head_index(id, gid, q, n);
  uint tail = q_get_tail_index(id, gid, q, n);
  return (tail > head) ? (tail - head) : QUEUE_SIZE - head;
}

/* Read the value located at the head index of the queue into 'result'.
 * Returns true if succcessful (queue is not empty), false otherwise. */
bool q_read(uint2 *result, size_t id, __global uint2 *q, int n) {
  size_t gid = get_global_id(0);
  if (q_is_empty(gid, id, q, n)) {
    return false;
  }

  int index = q_get_head_index(gid, id, q, n);
  *result = q[(n*n) + (gid * n * QUEUE_SIZE) + (id * QUEUE_SIZE) + index];
  q_set_head_index((index + 1) % QUEUE_SIZE, gid, id, q, n);
  q_set_last_op(READ, gid, id, q, n);
  return true;
}

/* Write a value into the tail index of the queue.
 * Returns true if successful (queue is not full), false otherwise. */
bool q_write(uint2 value, size_t id, __global uint2 *q, int n) {
  size_t gid = get_global_id(0);
  if (q_is_full(id, gid, q, n)) {
    return false;
  }
  
  int index = q_get_tail_index(id, gid, q, n);
  q[(n*n) + (id * n * QUEUE_SIZE) + (gid * QUEUE_SIZE) + index] = value;
  q_set_tail_index((index + 1) % QUEUE_SIZE, id, gid, q, n);
  q_set_last_op(WRITE, id, gid, q, n);
  return true;
}

/**************************/
/**** Packet Functions ****/
/**************************/

/* Return a newly created packet. */
packet pkt_create(uint type, uint source, uint arg, uint sub, uint payload) {
  packet p;
  pkt_set_type(&p, type);
  pkt_set_source(&p, source);
  pkt_set_arg_pos(&p, arg);
  pkt_set_sub(&p, sub);
  pkt_set_payload_type(&p, VAL);
  pkt_set_payload(&p, payload);
  return p;
}

/* Return the packet type. */
uint pkt_get_type(packet p) {
  return (p.x & PKT_TYPE_MASK) >> PKT_TYPE_SHIFT;
}

/* Return the packet source address. */
uint pkt_get_source(packet p) {
  return (p.x & PKT_SRC_MASK) >> PKT_SRC_SHIFT;
}

/* Return the packet argument position. */
uint pkt_get_arg_pos(packet p) {
  return (p.x & PKT_ARG_MASK) >> PKT_ARG_SHIFT;
}

/* Return the packet subtask. */
uint pkt_get_sub(packet p) {
  return (p.x & PKT_SUB_MASK) >> PKT_SUB_SHIFT;
}

/* Return the packet payload type. */
uint pkt_get_payload_type(packet p) {
  return (p.x & PKT_PTYPE_MASK) >> PKT_PTYPE_SHIFT;
}

/* Return the packet payload. */
uint pkt_get_payload(packet p) {
  return p.y;
}

/* Set the packet type. */
void pkt_set_type(packet *p, uint type) {
  (*p).x = ((*p).x & ~PKT_TYPE_MASK) | ((type << PKT_TYPE_SHIFT) & PKT_TYPE_MASK);
}

/* Set the packet source address. */
void pkt_set_source(packet *p, uint source) {
  (*p).x = ((*p).x & ~PKT_SRC_MASK) | ((source << PKT_SRC_SHIFT) & PKT_SRC_MASK);
}

/* Set the packet argument position. */
void pkt_set_arg_pos(packet *p, uint arg) {
  (*p).x = ((*p).x & ~PKT_ARG_MASK) | ((arg << PKT_ARG_SHIFT) & PKT_ARG_MASK);
}

/* Set the packet subtask. */
void pkt_set_sub(packet *p, uint sub) {
  (*p).x = ((*p).x & ~PKT_SUB_MASK) | ((sub << PKT_SUB_SHIFT) & PKT_SUB_MASK);
}

/* Set the packet payload type. */
void pkt_set_payload_type(packet *p, uint ptype) {
  (*p).x = ((*p).x & ~PKT_PTYPE_MASK) | ((ptype << PKT_PTYPE_SHIFT) & PKT_PTYPE_MASK);
}

/* Set the packet payload. */
void pkt_set_payload(packet *p, uint payload) {
  (*p).y = payload;
}
