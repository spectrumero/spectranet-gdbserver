#ifndef __UTILS_H
#define __UTILS_H

#include <stdint.h>

extern void print42(const char* text) __z88dk_callee __z88dk_fastcall;
extern void clear42();

extern void char_to_hex(char* res, uint8_t c) __z88dk_fastcall __z88dk_callee;
extern uint8_t hex_to_char(const char* from) __z88dk_fastcall __z88dk_callee;

extern uint16_t from_hex_str(const char* in, uint8_t len) __z88dk_callee;
extern void from_hex(const char* in, uint8_t* out, uint8_t len) __z88dk_callee;
extern void to_hex(const uint8_t* in, char* out, uint8_t len) __z88dk_callee;

#endif