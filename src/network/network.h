#ifndef STRONK_NETWORK_H
#define STRONK_NETWORK_H

#include <stdint.h>

#include <mcpr/mcpr.h>
#include <mcpr/abstract_packet.h>

void net_tick(void);
int net_init(void);
void net_cleanup(void);

int net_on_packet(uint8_t pkt_id, enum mcpr_state state, void (*handle)(struct mcpr_abstract_packet *pkt));

#endif
