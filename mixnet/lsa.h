#ifndef LSA_H_
#define LSA_H_

#include "packet.h"

#define LSA_NEIGHBOR_DISCOVERY 1
#define LSA_ADD_AND_SEND_NEIGHBORS 2
#define LSA_CONSTRUCT 3
#define LSA_RUN 4

void lsa_init();
void lsa_free();
void lsa_add_neighbors();
void lsa_send_neighbors();
int lsa_update(mixnet_packet_lsa *lsa_packet);
int lsa_broadcast();
void lsa_update_status();

#endif // LSA_H_
