#ifndef __STATE_H
#define __STATE_H

#include <stdint.h>

/*
 * Beware, only 1022 bytes are available
 */
struct gdbserver_state_t
{
    int server_socket;
    int client_socket;
    uint8_t buffer[128];
    uint8_t w_buffer[128];
};

extern struct gdbserver_state_t gdbserver_state;

#endif