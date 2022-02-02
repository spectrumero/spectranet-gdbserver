include "../include/spectranet.inc"
include "../include/sysvars.inc"

global _gdbserver_state
global t_rst8_handler
global __debug_framepointer
global CONSOLE_ROWS
global CONSOLE_COLUMNS
global __SYSVAR_BORDCR
global _clear42
global _print42
global module_header
extern gdbserver_install

defc INITIAL_SP = 0xFFFF
defc _print42 = PRINT42
defc _clear42 = CLEAR42
defc __SYSVAR_BORDCR = 23624
defc CONSOLE_ROWS = 24
defc CONSOLE_COLUMNS = 40
defc __debug_framepointer = 0x3B00

include "vars.inc"
# export to C
defc _gdbserver_state = gdbserver_state

global _set_trap
defc _set_trap = SETTRAP
defc _reset_trap = DISABLETRAP

module_header:
    org 0x2000
    defb 0xAA               ; This is a code module.
    defb 0xBC               ; This module has the identity 0xBC.
    defw reset              ; The RESET vector - call a routine labeled reset.
    defw 0xFFFF             ; MOUNT vector - not used by this module
    defw 0xFFFF             ; Reserved
    defw nmi_handler        ; Address of NMI routine
    defw 0xFFFF             ; Reserved
    defw 0xFFFF             ; Reserved
    defw STR_identity       ; Address of the identity string.

global modulecall
modulecall:
    call F_savescreen
    extern _modulecall
    call _modulecall
    call F_restorescreen
    ret

nmi_handler:
    ld a, (v_rst8_handler)
    jr z, nmi_handler_done

    # sp
    ld hl, (NMISTACK)
    ld (gdbserver_register_sp), hl

    # pc
    ld c, (hl)
    inc hl
    ld b, (hl)
    ld (gdbserver_register_pc), bc

    # rest of registers
    ld hl, (NMISTACK-6)
    ld (gdbserver_register_hl), hl
    ld hl, (NMISTACK-8)
    ld (gdbserver_register_de), hl
    ld hl, (NMISTACK-10)
    ld (gdbserver_register_bc), hl
    ld hl, (NMISTACK-12)
    ld (gdbserver_register_af), hl

    call modulecall

    ld hl, (gdbserver_register_sp)
    ld (NMISTACK), hl

    # pc
    ld a, (gdbserver_register_pc)
    ld (hl), a
    inc hl
    ld a, (gdbserver_register_pc + 1)
    ld (hl), a

    # rest of registers
    ld hl, (gdbserver_register_hl)
    ld (NMISTACK-6), hl
    ld hl, (gdbserver_register_de)
    ld (NMISTACK-8), hl
    ld hl, (gdbserver_register_bc)
    ld (NMISTACK-10), hl
    ld hl, (gdbserver_register_af)
    ld (NMISTACK-12), hl

nmi_handler_done:
    ret

STR_identity:
    defb "gdbserver", 0

STR_gdbserver:
    defb "%gdbserver", 0

basic_ext:
    defb 0x0B			    ; C Nonsense in BASIC
    defw STR_gdbserver	    ; Pointer to string (null terminated)
    defb 0xFF			    ; This module
    defw gdbserver_install	; Address of routine to call

reset:
    ld hl, basic_ext	    ; Pointer to the table entry to add
    call ADDBASICEXT
    ret

F_savescreen:
    ld bc, CTRLREG		; save border colour
    in a, (c)
    and 7
    ld (v_border), a
	ld a, (v_pga)		; save page A value
    ld (v_pr_pga), a

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
	ld a, (v_pr_pga)
    call SETPAGEA
    ld a, (v_border)
    out (254), a
	ret