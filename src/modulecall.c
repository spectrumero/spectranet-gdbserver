#include "server.h"
#include <arch/zx/spectrum.h>
#include "utils.h"
#include "state.h"
#include "sockpoll.h"

static void resume_execution()
{
    print42("resuming execution\n");
}

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
    resume_execution();
}
