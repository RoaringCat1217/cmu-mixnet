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

    uint8_t port = -1;
    if (curr_hop_index < routing_header->route_length) {
        port = find_port(routing_header->route[curr_hop_index], neighbor_addrs,
                         node_config.num_neighbors);
    } else {
        port = find_port(routing_header->dst_address, neighbor_addrs,
                         node_config.num_neighbors);
    }

    void *sendbuf = malloc(total_size);
    memcpy(sendbuf, packet_recv_ptr, total_size);
    mixnet_packet *headerp = (mixnet_packet *)sendbuf;

    mixnet_packet_routing_header *new_routing_header =
        (mixnet_packet_routing_header *)((char *)sendbuf +
                                         sizeof(mixnet_packet));
    new_routing_header->hop_index = curr_hop_index;

    pack_pending_packet(port, headerp);

    print("forwarded packet to port %d (node %d), pushed to queue", port,
          neighbor_addrs[port]);

    return 0;
}

int ping_send(mixnet_packet_routing_header *header, bool type) {
    if (type == PING_REQUEST) {
        mixnet_address dst = header->dst_address;
        path *routing_path = NULL;
        if (node_config.do_random_routing) {
            // TODO: get a random route
        } else {
            routing_path = shortest_paths[dst];
        }
        linkedlist *route = routing_path->route;
        uint16_t total_size = sizeof(mixnet_packet) +
                              sizeof(mixnet_packet_routing_header) +
                              (route->size - 2) * sizeof(mixnet_address) +
                              sizeof(mixnet_packet_ping);
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
        mixnet_packet_ping *ping_recv =
            (mixnet_packet_ping *)((char *)header +
                                   sizeof(mixnet_packet_routing_header));
        ping->is_request = PING_REQUEST;
        ping->send_time = ping_recv->send_time;

        pack_pending_packet(routing_path->sendport, mixnet_header);
        char route_str[512];
        ll_print(route_str, route->head->next, route->tail);
        print("added PING packet with route %s and port %d to queue", route_str,
              routing_path->sendport);

        if (node_config.do_random_routing) {
            path_free(routing_path);
        }
    } else {
        mixnet_address dst = header->src_address;
        uint16_t total_size = sizeof(mixnet_packet) +
                              sizeof(mixnet_packet_routing_header) +
                              header->route_length * sizeof(mixnet_address) +
                              sizeof(mixnet_packet_ping);
        char *sendbuf = malloc(total_size);

        mixnet_packet *mixnet_header = (mixnet_packet *)sendbuf;
        mixnet_header->total_size = total_size;
        mixnet_header->type = PACKET_TYPE_PING;

        mixnet_packet_routing_header *routing_header =
            (mixnet_packet_routing_header *)(sendbuf + sizeof(mixnet_packet));
        routing_header->src_address = node_config.node_addr;
        routing_header->dst_address = dst;
        routing_header->route_length = header->route_length;
        routing_header->hop_index = 0;
        for (uint16_t i = 0; i < routing_header->route_length; i++) {
            routing_header->route[i] =
                header->route[routing_header->route_length - 1 - i];
        }

        mixnet_packet_ping *ping =
            (mixnet_packet_ping *)(sendbuf + sizeof(mixnet_packet) +
                                   sizeof(mixnet_packet_routing_header) +
                                   header->route_length *
                                       sizeof(mixnet_address));
        mixnet_packet_ping *ping_recv =
            (mixnet_packet_ping *)((char *)header +
                                   sizeof(mixnet_packet_routing_header) +
                                   header->route_length *
                                       sizeof(mixnet_address));
        ping->is_request = PING_RESPONSE;
        ping->send_time = ping_recv->send_time;

        mixnet_address next = routing_header->route[0];
        uint8_t port = 0;
        for (port = 0; port < node_config.num_neighbors; port++)
            if (neighbor_addrs[port] == next)
                break;

        pack_pending_packet(port, mixnet_header);

        char route_str[512];
        char *print_head = route_str;
        for (int i = 0; i < routing_header->route_length - 1; i++)
            print_head += sprintf(print_head, "%d->", routing_header->route[i]);
        sprintf(print_head, "%d",
                routing_header->route[routing_header->route_length - 1]);
        print("added PING packet with route %s and port %d to queue", route_str,
              port);
    }

    return 0;
}

int ping_recv(mixnet_packet_routing_header *header) {
    if (port_recv == node_config.num_neighbors) {
        ping_send(header, PING_REQUEST);
    } else {
        mixnet_packet_ping *ping =
            (mixnet_packet_ping *)((char *)header +
                                   sizeof(mixnet_packet_routing_header) +
                                   header->route_length *
                                       sizeof(mixnet_address));
        if (ping->is_request)
            ping_send(header, PING_RESPONSE);
    }
    return 0;
}

int data_send(mixnet_packet_routing_header *header) {
    mixnet_address dst = header->dst_address;
    path *routing_path = NULL;
    if (node_config.do_random_routing) {
        // TODO: get a random route
    } else {
        routing_path = shortest_paths[dst];
    }
    linkedlist *route = routing_path->route;
    uint16_t total_size = packet_recv_ptr->total_size +
                          (route->size - 2) * sizeof(mixnet_address);
    uint16_t old_offset =
        sizeof(mixnet_packet) + sizeof(mixnet_packet_routing_header);
    uint16_t new_offset = sizeof(mixnet_packet) +
                          sizeof(mixnet_packet_routing_header) +
                          (route->size - 2) * sizeof(mixnet_address);
    uint16_t data_size = packet_recv_ptr->total_size - old_offset;

    char *sendbuf = malloc(total_size);

    mixnet_packet *mixnet_header = (mixnet_packet *)sendbuf;
    mixnet_header->total_size = total_size;
    mixnet_header->type = PACKET_TYPE_DATA;

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

    char *src_offset_ptr = ((char *)packet_recv_ptr) + old_offset;
    char *dest_offset_ptr = ((char *)sendbuf) + new_offset;

    memcpy(dest_offset_ptr, src_offset_ptr, data_size);

    pack_pending_packet(routing_path->sendport, mixnet_header);

    char route_str[512];
    ll_print(route_str, route->head->next, route->tail);
    print("added DATA packet with route %s and port %d to queue", route_str,
          routing_path->sendport);
    print("packet data: %s", (char *)mixnet_header + new_offset);

    if (node_config.do_random_routing) {
        path_free(routing_path);
    }

    return 0;
}

int data_recv(mixnet_packet_routing_header *header) {
    if (port_recv == node_config.num_neighbors) {
        data_send(header);
    }

    return 0;
}

void pack_pending_packet(uint8_t port, mixnet_packet *headerp) {
    port_and_packet *pending_packet = malloc(sizeof(port_and_packet));
    pending_packet->port = port;
    pending_packet->packet = headerp;
    pending_packets[curr_mixing_count] = pending_packet;
    curr_mixing_count++;
}
