#ifndef LSA_H_
#define LSA_H_

#include "packet.h"

#define LSA_NEIGHBOR_DISCOVERY 1
#define LSA_ADD_NEIGHBORS_AND_SEND 2
#define LSA_CONSTRUCT 3
#define LSA_RUN 4

void lsa_init();
void lsa_free();
int lsa_update(mixnet_packet_lsa *lsa_packet);
int lsa_flood();
int lsa_broadcast();
void lsa_update_status();

#endif // LSA_H_
