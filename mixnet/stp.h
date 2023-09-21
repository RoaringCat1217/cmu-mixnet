#ifndef STP_H_
#define STP_H_

#include "packet.h"

int stp_send();
int stp_hello();
int stp_recv(mixnet_packet_stp *stp_packet);
int stp_check_timer();
int stp_flood();

#endif // STP_H_
