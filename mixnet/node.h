/**
 * Copyright (C) 2023 Carnegie Mellon University
 *
 * This file is part of the Mixnet course project developed for
 * the Computer Networks course (15-441/641) taught at Carnegie
 * Mellon University.
 *
 * No part of the Mixnet project may be copied and/or distributed
 * without the express permission of the 15-441/641 course staff.
 */
#ifndef MIXNET_NODE_H_
#define MIXNET_NODE_H_

#include "address.h"
#include "config.h"
#include "connection.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void run_node(void *const handle,
              volatile bool *const keep_running,
              const struct mixnet_node_config c);

void init_node(struct mixnet_node_config c,
               mixnet_address* mixnet_addr,
               mixnet_packet_stp *config,
               mixnet_address *neighbor_addrs,
               bool *port_status);

void run_stp(mixnet_packet_stp *config, mixnet_packet *packet, bool *port_status, int port_id);

#ifdef __cplusplus
}
#endif

#endif // MIXNET_NODE_H_
