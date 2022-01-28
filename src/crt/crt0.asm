defc CLEAR42 = 0x3E30
defc PRINT42 = 0x3E2D
defc GETKEY = 0x3E66
defc ADDBASICEXT = 0x3E93
defc IXCALL = 0x3FFD
defc STATEMENT_END = 0x3E96
defc EXIT_SUCCESS = 0x3E99
defc SETPAGEA = 0x3E33
defc SETPAGEB = 0x3E36
defc CTRLREG = 0x033B

defc SETTRAP = 0x3E78
defc ENABLETRAP = 0x3E7E

defc v_port7ffd = 0x3FEE
defc v_border = 0x3FEF

defc INITIAL_SP = 0xFFFF

global _print42
defc _print42 = PRINT42

global _clear42
defc _clear42 = CLEAR42

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
    defb "To activate debugger, press NMI button.\n", 0

basic_ext:
    defb 0x0B			; C Nonsense in BASIC
    defw STR_gdbserver	; Pointer to string (null terminated)
    defb 0xFF			; This module
    defw start			; Address of routine to call

nmi:
    extern _nmi
    call F_savescreen
	ld bc, 0xFFFD		; silence the AY (even for 48K machines
	ld a, 8			; in case they have an AY add on compatible
	out (c), a		; with the 128K)
	ld b, 0xBF		; set register 8
	xor a			; to 0
	out (c), a
	ld b, 0xFF
	ld a, 9			; register 9 to zero
	out (c), a
	ld b, 0xBF
	xor a
	out (c), a
	ld a, 10		; register 10 to zero
	ld b, 0xFF
	out (c), a
	ld b, 0xBF
	xor a
	out (c), a
    call _nmi
    call F_restorescreen
	ld a, (v_port7ffd)	; Restore port 0x7FFD
	ld bc, 0x7ffd
	out (c), a
	ld a, (v_border)	; Restore the border colour
	out (254), a
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

F_savescreen:
	ld bc, CTRLREG		; save border colour
	in a, (c)
	and 7
	ld (v_border), a

	ld a, 0xDA		; Use pages 0xDA, 0xDB of RAM
	call SETPAGEA
	ld hl, 0x4000		; Spectrum screen buffer
	ld de, 0x1000		; Page area A
	ld bc, 0x1000		; 4K
	ldir
	ld a, 0xDB
	call SETPAGEA
	ld hl, 0x5000
	ld de, 0x1000
	ld bc, 0xB00		; Remainder of screen, including attrs.
	ldir
	ret

F_restorescreen:
	ld a, 0xDA
	call SETPAGEA
	ld hl, 0x1000
	ld de, 0x4000
	ld bc, 0x1000
	ldir
	ld a, 0xDB
	call SETPAGEA
	ld hl, 0x1000
	ld de, 0x5000
	ld bc, 0xB00
	ldir
	ret