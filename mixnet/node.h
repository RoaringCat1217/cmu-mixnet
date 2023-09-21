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
#include "packet.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_node();
void free_node();
void run_node(void *const handle, volatile bool *const keep_running,
              const struct mixnet_node_config c);
int send_to_user();

#ifdef __cplusplus
}
#endif

#endif // MIXNET_NODE_H_
