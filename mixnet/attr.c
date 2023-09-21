#include "attr.h"

#define ADDR_UNK -1
#define NO_NEXTHOP -1
#define INT_MAX 0x7FFFFFFF

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
graph *g;
path **shortest_paths;

// receive packets
uint8_t port_recv;
mixnet_packet *packet_recv_ptr;
bool need_free;
