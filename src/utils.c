#include "utils.h"
#include "string.h"

void from_hex(const char* in, uint8_t* out, uint8_t len) __z88dk_callee
{
    const char* end = in + len;
    while (in < end)
    {
        *out++ = hex_to_char(in);
        in += 2;
    }
}

void to_hex(const uint8_t* in, char* out, uint8_t len) __z88dk_callee
{
    while (len--)
    {
        char_to_hex(out, *in++);
        out += 2;
    }
}

uint16_t from_hex_str(const char* in, uint8_t len) __z88dk_callee
{
    char b[4];
    memset(b, '0', 4);
    memcpy(b + 4 - len, in, len);

    uint8_t result[2];
    from_hex(b, result, 4);

    uint16_t result_v;

    *(uint8_t*)&result_v = result[1];
    *((uint8_t*)&result_v + 1) = result[0];

    return result_v;
}