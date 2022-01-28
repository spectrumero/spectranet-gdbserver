#ifndef __SERVER_H
#define __SERVER_H

#include <stdint.h>

extern uint8_t server_init();
extern uint8_t server_listen();
extern uint8_t server_iteration();

#endif