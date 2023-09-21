#include "attr.h"

// states of the node should be initialized as global variables

// basic states
void *myhandle;
struct mixnet_node_config node_config;
uint8_t lsa_status;

// stp
mixnet_address *neighbor_addrs;
mixnet_packet_stp stp_curr_state;
uint8_t stp_nexthop;
bool *port_open;
int *dist_to_root;
unsigned long timer;

// lsa
graph *g;
path **shortest_paths;

// receive packets
uint8_t port_recv;
mixnet_packet *packet_recv_ptr;
bool need_free;
