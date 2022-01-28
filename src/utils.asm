
# ================================================================================================
# extern uint8_t hex_to_char(const char* from) __z88dk_fastcall __z88dk_callee;
# ================================================================================================
public _hex_to_char

# hl contains a pointer to two chars
# result is returned through register l
_hex_to_char:
    ld   a,(hl)
    inc  hl
    call hex1
    add  a,a
    add  a,a
    add  a,a
    add  a,a
    ld   d,a
    ld   a,(hl)
    call hex1
    and  0x0F
    or   d
    ld   h, 0
    ld   l, a
    ret

hex1:
    # 0..9
    sub  a, '0'
    cp   10
    ret  c
    # A..F
    sub  a, 0x07
    cp   16
    ret  c
    # a..f
    sub  a, 0x20
    ret


# ================================================================================================
# extern void char_to_hex(char* res, uint8_t c) __z88dk_fastcall __z88dk_callee;
# ================================================================================================
public _char_to_hex

_char_to_hex:
    # ret
    pop bc
    # char* res
    pop de
    # restore ret
    push bc
    ld  h, l
    ld  a, l
    rra
    rra
    rra
    rra
    and 0x0F
    call _hex_a_to_hex
    ld (de), a
    inc de
    ld a, l
    and 0x0F
    call _hex_a_to_hex
    ld (de), a
    ret
_hex_a_to_hex:
    cp  10
    jp nc, _hex_af
    add a, '0'
    ret
_hex_af:
    add a, 'A'-10
    ret