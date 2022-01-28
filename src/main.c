#include "server.h"
#include "spectranet.h"
#include "state.h"
#include <arch/zx/spectrum.h>

void nmi()
{
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
