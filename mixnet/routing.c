#include "routing.h"
#include "attr.h"
#include "packet.h"
#include "utils.h"

void pack_pending_packet(uint8_t port, mixnet_packet *headerp);

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

    pack_pending_packet(port, headerp);

    return 0;
}

void pack_pending_packet(uint8_t port, mixnet_packet *headerp) {
    port_and_packet *pending_packet = malloc(sizeof(port_and_packet));
    pending_packet->port = port;
    pending_packet->packet = headerp;
    pending_packets[curr_mixing_count] = pending_packet;
}

void ping_send(mixnet_packet_routing_header *header, bool type) {
    mixnet_address dst = header->dst_address;
    path *routing_path;
    if (node_config.do_random_routing) {
        // TODO: get a random route
    } else {
        routing_path = shortest_paths[dst];
    }
    linkedlist *route = routing_path->route;
    uint16_t total_size =
        sizeof(mixnet_packet) + sizeof(mixnet_packet_routing_header) +
        (route->size - 2) * sizeof(mixnet_address) + sizeof(mixnet_packet_ping);
    char *sendbuf = malloc(total_size);

    mixnet_packet *mixnet_header = (mixnet_packet *)sendbuf;
    mixnet_header->total_size = total_size;
    mixnet_header->type = PACKET_TYPE_PING;

    mixnet_packet_routing_header *routing_header =
        (mixnet_packet_routing_header *)(sendbuf + sizeof(mixnet_packet));
    routing_header->src_address = node_config.node_addr;
    routing_header->dst_address = dst;
    routing_header->route_length = (uint16_t)(route->size - 2);
    routing_header->hop_index = 0;
    int i = 0;
    for (ll_node *ptr = route->head->next; ptr != route->tail;
         ptr = ptr->next) {
        routing_header->route[i++] = ptr->node_addr;
    }

    mixnet_packet_ping *ping =
        (mixnet_packet_ping *)(sendbuf + sizeof(mixnet_packet) +
                               sizeof(mixnet_packet_routing_header) +
                               (route->size - 2) * sizeof(mixnet_address));
    ping->is_request = type;
    ping->send_time = get_timestamp(MICROSEC);
    pack_pending_packet(routing_path->sendport, mixnet_header);

    if (node_config.do_random_routing)
        path_free(routing_path);
}
