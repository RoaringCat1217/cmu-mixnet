#ifndef UTIL_H_
#define UTIL_H_

int mixnet_send_loop(void *handle, const uint8_t port, mixnet_packet *packet);
unsigned long get_timestamp();

#endif // UTIL_H_
