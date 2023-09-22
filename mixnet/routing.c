#include "attr.h"
#include "packet.h"
#include "utils.h"

int routing_forward(char *payload) {
    mixnet_packet_routing_header *routing_header =
        (mixnet_packet_routing_header *)payload;
    int curr_hop_index = routing_header->hop_index + 1;
    int total_size = packet_recv_ptr->total_size;

    uint8_t port = find_port(routing_header->route[curr_hop_index],
                             neighbor_addrs, node_config.num_neighbors);

    void *sendbuf = malloc(total_size);
    memcpy(sendbuf, packet_recv_ptr, total_size);
    mixnet_packet *headerp = (mixnet_packet *)sendbuf;

    mixnet_packet_routing_header *new_routing_header =
        (mixnet_packet_routing_header *)((char *)sendbuf +
                                         sizeof(mixnet_packet));
    new_routing_header->hop_index = curr_hop_index;

    port_and_packet *pending_packet = malloc(sizeof(port_and_packet));
    pending_packet->port = port;
    pending_packet->packet = headerp;

    pending_packets[curr_hop_index] = pending_packet;

    return 0;
}
