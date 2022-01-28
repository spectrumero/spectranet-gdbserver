#include "server.h"
#include <arch/zx/spectrum.h>
#include "utils.h"

void nmi()
{
    zx_border(INK_BLUE);
    zx_colour(INK_WHITE | PAPER_BLUE);

    clear42();
    print42("gdbserver by @desertkun\n");

    if (server_init())
    {
        return;
    }

    if (server_listen())
    {
        return;
    }

    while (server_iteration()) ;
}
