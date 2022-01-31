include "../include/spectranet.inc"

global _set_trap
_set_trap:
    call SETTRAP
    call ENABLETRAP
    ret

global _reset_trap
_reset_trap:
    call DISABLETRAP
    ret
