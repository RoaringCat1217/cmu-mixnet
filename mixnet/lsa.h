#ifndef LSA_H_
#define LSA_H_

void lsa_init();
void lsa_free();
int lsa_update(mixnet_packet_lsa *lsa_packet);
int lsa_flood();

#endif // LSA_H_