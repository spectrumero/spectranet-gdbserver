include "../include/spectranet.inc"
include "../include/sysvars.inc"

defc gdbserver_state = 0x3B02
defc gdbserver_registers = gdbserver_state + 0

defc gdbserver_register_sp = gdbserver_registers
defc gdbserver_register_pc = gdbserver_registers + 2
defc gdbserver_register_hl = gdbserver_registers + 4
defc gdbserver_register_de = gdbserver_registers + 6
defc gdbserver_register_bc = gdbserver_registers + 8
defc gdbserver_register_af = gdbserver_registers + 10

defc gdbserver_rst8_handler = gdbserver_state + 12
defc gdbserver_sockets = gdbserver_rst8_handler + 64

# this is just a blob of data, this is never called directly
# when it is executed, it is located at 0x3B02
rst8h_handler_src:
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

    # restore hl
    ld hl, (gdbserver_register_hl)

    # pc is on the stack
    ret
rst8h_handler_src_end:

global gdbserver_install
gdbserver_install:
    call STATEMENT_END	; Check for statement end.

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

    # trip ourselves
    rst 0x08

    jp EXIT_SUCCESS