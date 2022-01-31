#include "server.h"
#include <arch/zx/spectrum.h>
#include "utils.h"
#include "state.h"
#include "sockpoll.h"

extern void restore_rst08h();

void modulecall()
{
    zx_border(INK_BLUE);
    zx_colour(INK_WHITE | PAPER_BLUE);

    clear42();
    print42("gdbserver by @desertkun\n");

    if (server_init())
    {
        return;
    }

    struct breakpoint_t* trapped_breakpoint = NULL;

    if (gdbserver_state.client_socket)
    {
        print42("execution stopped\n");
        uint16_t* pc = &gdbserver_state.registers[REGISTERS_PC];

        // unwind RST08
        (*pc)--;

        for (uint8_t i = 0; i < MAX_BREAKPOINTS_COUNT; i++)
        {
            struct breakpoint_t* b = &gdbserver_state.breakpoints[i];
            if (b->address == *pc)
            {
                trapped_breakpoint = b;

                // restore original instruction
                *(uint8_t*)b->address = b->original_instruction;
                break;
            }
        }

        // report we have trapped
        server_write_packet("T05thread:p01.01;");
    }
    else
    {
        if (server_listen())
        {
            return;
        }
    }

    while (1)
    {
        switch (poll_fd(gdbserver_state.client_socket))
        {
            case POLLHUP:
            {
                server_on_disconnect();
                continue;
            }
            case POLLIN:
            {
                if (server_read_data())
                {
                    continue;
                }
                else
                {
                    goto done;
                }
            }
            default:
            {
                continue;
            }
        }
    }

done:
    print42("resuming execution\n");

    // we have to restore RST08 on the breakpoint
    if (trapped_breakpoint && trapped_breakpoint->address == gdbserver_state.registers[REGISTERS_PC])
    {
        gdbserver_state.trap_handler.page = 0xFF;
        gdbserver_state.trap_handler.address = trapped_breakpoint->address;
        gdbserver_state.trap_handler.next_address = 0x0000;
        gdbserver_state.trap_handler.handler = (uint16_t)restore_rst08h;

        set_trap(&gdbserver_state.trap_handler);
    }
}
