#ifndef __SERVER_H
#define __SERVER_H

#include <stdint.h>

extern uint8_t server_init();
extern uint8_t server_listen();
extern void server_on_disconnect();
extern uint8_t server_read_data();
extern void server_write_packet(const char *data) __z88dk_fastcall;

#endif