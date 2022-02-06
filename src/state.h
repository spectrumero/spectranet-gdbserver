#ifndef __STATE_H
#define __STATE_H

#include <stdint.h>

#define MAX_BREAKPOINTS_COUNT (8)
#define REGISTERS_COUNT (6)

#define REGISTERS_SP (0)
#define REGISTERS_PC (1)
#define REGISTERS_HL (2)
#define REGISTERS_DE (3)
#define REGISTERS_BC (4)
#define REGISTERS_AF (5)

struct breakpoint_t {
    uint16_t address;
    uint8_t original_instruction;
};

/*
 * Beware, only 1021 bytes are available
 * Offsets of these are crucial
 */
struct gdbserver_state_t
{
    // Offset: 0
    uint16_t registers[REGISTERS_COUNT]; /* sp, pc, hl, de, bc, af */
    // Offset: 12
    uint8_t rst8_handler[80];
    // Offset: 92
    int server_socket;
    // Offset: 94
    int client_socket;
    // Offset: 96
    struct {
        uint8_t page;
        uint16_t handler;
        uint16_t next_address;
        uint16_t address;
    } trap_handler;
    // Offset: 103
    uint8_t trap_flags;
    // Offsets of these is not important
    uint8_t buffer[128];
    uint8_t w_buffer[128];
    struct breakpoint_t breakpoints[8];
    struct breakpoint_t temporary_breakpoint;
};

#define TRAP_FLAG_RESTORE_RST08H (0x01)
#define TRAP_FLAG_STEP_INSTRUCTION (0x02)

extern struct gdbserver_state_t gdbserver_state;

#endif