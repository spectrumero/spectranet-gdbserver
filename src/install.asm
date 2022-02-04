include "../include/spectranet.inc"
include "../include/sysvars.inc"

include "vars.inc"

# this is just a blob of data, this is never called directly
# when it is executed, it is located at 0x3B02
rst8h_handler_src:
    ld hl, v_rst8_handler
    ld (hl), 1

    # restore af undef name of hl
    pop hl
    ld (gdbserver_register_af), hl

    # store sp
    ld (gdbserver_register_sp), sp

    # store pc
    pop de
    ld (gdbserver_register_pc), de

    # store hl
    ld hl, (v_hlsave)
    ld (gdbserver_register_hl), hl

    # store de
    ld de, (v_desave)
    ld (gdbserver_register_de), de

    # store bc
    ld (gdbserver_register_bc), bc

    # call our module
    ld hl, 0xBC00
    rst MODULECALL_NOPAGE

    # restore main registers
    ld sp, (gdbserver_register_af + 2)
    pop af
    pop bc
    pop de
    pop hl

    # sp could have changed
    ld sp, (gdbserver_register_sp)

    # restore af, bc, de
    pop af
    pop bc
    pop de

    # sp could have changed
    ld sp, (gdbserver_registers)

    # restore pc by pushing it on the stack
    ld hl, (gdbserver_register_pc)
    push hl

    ld hl, v_rst8_handler
    ld (hl), 0

    # restore hl
    ld hl, (gdbserver_register_hl)

    # unpage, its going to call ret for us
    jp 0x007C
rst8h_handler_src_end:

STR_installed:
    defb "*** GDBSERVER INSTALLED ***\nPress NMI, then attach on port 1667.\n", 0

STR_install_error:
    defb "Cannot install. Make sure your spectranet firmware is updated.\n", 0

global gdbserver_install
gdbserver_install:
    call STATEMENT_END	; Check for statement end.

    ld a, (v_nmipage)
    cp 0x02
    jp z, gdbserver_install_ok

    ld hl, STR_install_error
    call PRINT42
    jp EXIT_SUCCESS

gdbserver_install_ok:
    # copy rst 8 handler
    ld hl, rst8h_handler_src
    ld de, gdbserver_rst8_handler
    ld bc, rst8h_handler_src_end - rst8h_handler_src
    ldir

    # clear gdbserver_state.server_socket
    ld hl, gdbserver_sockets
    ld (hl), 0x00
    inc hl
    ld (hl), 0x00
    inc hl
    # clear gdbserver_state.client_socket
    ld (hl), 0x00
    inc hl
    ld (hl), 0x00

    # handle rst 8
    ld hl, gdbserver_rst8_handler
    ld (v_rst8vector), hl

    # handle nmi
    ld a, (v_pgb)
    ld (v_nmipage), a

    ld hl, STR_installed
    call PRINT42

    jp EXIT_SUCCESS

global _restore_rst08h
_restore_rst08h:
    # load instruction address
    ld hl, (gdbserver_trap_handler+5)
    # put rst 08 back
    ld (hl), 0xCF
    # one time only
    call DISABLETRAP
    jp PAGETRAPRETURN