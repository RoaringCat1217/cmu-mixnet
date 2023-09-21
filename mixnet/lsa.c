#include "attr.h"
#include "utils.h"
#include "packet.h"

#include <string.h>

int lsa_update() {
    return 0;
}

// send LSA packets along the spanning tree
int lsa_flood() {
    int ret = 0;

    for (uint8_t port_num = 0; port_num < node_config.num_neighbors;
         ++port_num) {
        if (port_num != port_recv && port_open[port_num]) {
            print("send lsa update to port %d(node %d)", port_num,
                  neighbor_addrs[port_num]);

            int n = node_config.num_neighbors;
            int header_size = sizeof(mixnet_packet);
            int lsa_size = sizeof(mixnet_packet_lsa);
            int lsa_link_size = sizeof(mixnet_lsa_link_params);
            int packet_size = header_size + lsa_size + n * lsa_link_size;
            
            void *sendbuf = malloc(packet_size);

            mixnet_packet *headerp = (mixnet_packet *)sendbuf;
            headerp->total_size = packet_size;
            headerp->type = PACKET_TYPE_LSA;

            mixnet_packet_lsa *payloadp =
                    (mixnet_packet_lsa *)((char *)sendbuf + header_size);

            mixnet_packet_lsa *payload = malloc(lsa_size + n * lsa_link_size);

            payload->node_address = node_config.node_addr;
            payload->neighbor_count = node_config.num_neighbors;
            for (int i = 0; i < n; ++i) {
                payload->links[i].neighbor_mixaddr = neighbor_addrs[i];
                payload->links[i].cost = 2;
            }

            memcpy(payloadp, payload, lsa_size + n * lsa_link_size);

            free(payload);

            ret = mixnet_send_loop(myhandle, port_num, sendbuf);
            if (ret < 0) {
                return -1;
            }
        }
    }

    return 0;
}