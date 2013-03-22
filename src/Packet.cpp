/* Packet functions are needed in both the host and kernel program. The purpose
 * of this file to provide the functions to the host */
#include "Packet.h"

/* Return a completely empty packet with all bits set to zero. */
packet pkt_base_init() {
  packet p;
  p.x = 0;
  p.y = 0;
  return p;
}

/* Return a newly created packet. */
packet pkt_create(uint type, uint source, uint arg, uint sub, uint payload) {
  packet p = pkt_base_init();
  pkt_set_type(&p, type);
  pkt_set_source(&p, source);
  pkt_set_arg_pos(&p, arg);
  pkt_set_sub(&p, sub);
  pkt_set_payload_type(&p, 0);
  pkt_set_payload(&p, payload);
  return p;
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
