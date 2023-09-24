#include "attr.h"

// states of the node should be initialized as global variables

// basic states
void *myhandle;
struct mixnet_node_config node_config;

// stp
mixnet_address *neighbor_addrs;
mixnet_packet_stp stp_curr_state;
uint8_t stp_nexthop;
bool *port_open;
int *dist_to_root;
unsigned long timer;

// lsa
uint8_t lsa_status;
graph *g;
path **shortest_paths;
unsigned long lsa_timer;
mixnet_address lsa_root;

// routing
uint16_t curr_mixing_count;
port_and_packet **pending_packets;

// receive packets
uint8_t port_recv;
mixnet_packet *packet_recv_ptr;
bool need_free;
