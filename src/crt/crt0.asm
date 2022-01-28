defc PRINT42 = 0x3E2D
defc GETKEY = 0x3E66
defc ADDBASICEXT = 0x3E93
defc IXCALL = 0x3FFD
defc STATEMENT_END = 0x3E96
defc EXIT_SUCCESS = 0x3E99

defc INITIAL_SP = 0xFFFF

global  __SYSVAR_BORDCR
defc    __SYSVAR_BORDCR = 23624

global  CONSOLE_ROWS
defc	CONSOLE_ROWS = 24
global  CONSOLE_COLUMNS
defc	CONSOLE_COLUMNS = 40

global __debug_framepointer
defc __debug_framepointer = 0x3B00

global _gdbserver_state
defc _gdbserver_state = 0x3B02


modile_header:
    org 0x2000
    defb 0xAA               ; This is a code module.
    defb 0xBC               ; This module has the identity 0xBC.
    defw reset              ; The RESET vector - call a routine labeled reset.
    defw 0xFFFF             ; MOUNT vector - not used by this module
    defw 0xFFFF             ; Reserved
    defw nmi                ; Address of NMI routine
    defw 0xFFFF             ; Reserved
    defw 0xFFFF             ; Reserved
    defw STR_identity       ; Address of the identity string.

STR_identity:
    defb "gdbserver", 0

STR_gdbserver:
    defb "%gdbserver", 0

STR_installed:
    defb "To activate debugger, press NMI button.", 0

basic_ext:
    defb 0x0B			; C Nonsense in BASIC
    defw STR_gdbserver	; Pointer to string (null terminated)
    defb 0xFF			; This module
    defw start			; Address of routine to call

nmi:
    extern _nmi
    call _nmi
    ret

start:
    call STATEMENT_END	; Check for statement end.
    # register ourselves as NMI handler
    ld a, (0x3F07)
    ld (0x3FF6), a
    # clear gdbserver_state.server_socket
    ld hl, 0x3B02
    ld (hl), 0x00
    inc hl
    ld (hl), 0x00
    inc hl
    # clear gdbserver_state.client_socket
    ld (hl), 0x00
    inc hl
    ld (hl), 0x00
    # done
    ld hl, STR_installed
    call PRINT42
    jp EXIT_SUCCESS

reset:
    ld hl, basic_ext	    ; Pointer to the table entry to add
    call ADDBASICEXT
    ret