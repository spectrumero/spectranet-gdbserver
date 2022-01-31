#include "server.h"
#include "sys/socket.h"
#include "sockpoll.h"
#include "utils.h"
#include "state.h"

#include <string.h>

uint8_t server_init()
{
    if (gdbserver_state.server_socket)
    {
        return 0;
    }

    gdbserver_state.server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (gdbserver_state.server_socket < 0)
    {
        return 1;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(1667);
    if (bind(gdbserver_state.server_socket, &address, sizeof(address)) < 0)
    {
        return 2;
    }

    return 0;
}

static void write_data_raw(const uint8_t *data, ssize_t len)
{
    send(gdbserver_state.client_socket, (void*)data, len, 0);
}

static void write_str_raw(const char *data)
{
    send(gdbserver_state.client_socket, (void*)data, strlen(data), 0);
}

static void write_packet_bytes(const uint8_t *data, uint8_t num_bytes) __z88dk_callee
{
    size_t i;

    char* wbuf = gdbserver_state.w_buffer;

    *wbuf++ = '$';
    memcpy(wbuf, data, num_bytes);
    wbuf += num_bytes;
    *wbuf++ = '#';

    uint8_t checksum;
    for (i = 0, checksum = 0; i < num_bytes; ++i)
        checksum += data[i];
    to_hex(&checksum, wbuf, 1);

    write_data_raw(gdbserver_state.w_buffer, num_bytes + 4);
}

void server_write_packet(const char *data) __z88dk_fastcall
{
    write_packet_bytes((const uint8_t *)data, strlen(data));
}

uint8_t server_listen()
{
    if (listen(gdbserver_state.server_socket, 1) < 0)
    {
        return 1;
    }

    print42("waiting for connections...\n");

    gdbserver_state.client_socket = accept(gdbserver_state.server_socket, NULL, NULL);
    if(gdbserver_state.client_socket <= 0)
    {
        return 2;
    }

    print42("connected!\n");

    return 0;
}

void server_on_disconnect()
{
    sockclose(gdbserver_state.client_socket);
    gdbserver_state.client_socket = 0;
    print42("client disconnected!\n");
    server_listen();
}

static const struct {
    const char* request;
    const char* response;
} queries[] = {
    {"Supported",               "PacketSize=128;NonBreakable;qXfer:features:read+;qXfer:auxv:read+"},
    {NULL,                      NULL},
};

static void write_error() __z88dk_callee
{
    server_write_packet("E01");
}

static void write_ok() __z88dk_callee
{
    server_write_packet("OK");
}

static uint8_t process_packet()
{
    char* payload = (char*)gdbserver_state.buffer;

    char command = *payload++;
    switch (command)
    {
        case 'c':
        {
            // continue execution
            return 0;
        }
        case 'q':
        {
            if (strstr(payload, "Xfer:features:read") == payload)
            {
                write_str_raw(
                    "$l<target version=\"1.0\"><feature name=\"org.gnu.gdb.z80.cpu\">"
                    "<reg name=\"sp\" bitsize=\"16\" type=\"data_ptr\"/>"
                    "<reg name=\"pc\" bitsize=\"16\" type=\"code_ptr\"/>"
                    "<reg name=\"hl\" bitsize=\"16\" type=\"int\"/>"
                    "<reg name=\"de\" bitsize=\"16\" type=\"int\"/>"
                    "<reg name=\"bc\" bitsize=\"16\" type=\"int\"/>"
                    "<reg name=\"af\" bitsize=\"16\" type=\"int\"/>"
                    "</feature><architecture>z80</architecture></target>#ba");
                return 1;
            }

            for (uint8_t i = 0; queries[i].request; i++)
            {
                if (strcmp(queries[i].request, payload) == 0)
                {
                    server_write_packet(queries[i].response);
                    return 1;
                }
            }

            goto error;
        }
        case '?':
        {
            // we're always stopped when we're under execution
            server_write_packet("T05thread:p01.01;");
            break;
        }
        case 'g':
        {
            // dump registers
            to_hex(gdbserver_state.registers, gdbserver_state.buffer, REGISTERS_COUNT * 2);
            write_packet_bytes(gdbserver_state.buffer, REGISTERS_COUNT * 4);
            break;
        }
        case 'G':
        {
            // set registers
            from_hex(payload, gdbserver_state.registers, REGISTERS_COUNT * 4);
            write_ok();
            break;
        }
        case 'm':
        {
            // read memory
            // 8000,38

            char* comma = strchr(payload, ',');
            if (comma == NULL)
            {
                goto error;
            }
            else
            {
                uint8_t* mem_offset = (uint8_t*)from_hex_str(payload, comma - payload);
                comma++;
                uint16_t mem_size = from_hex_str(comma, strlen(comma));
                to_hex(mem_offset, gdbserver_state.buffer, mem_size);
                write_packet_bytes(gdbserver_state.buffer, mem_size * 2);
            }
            break;
        }
        case 'M':
        {
            // write memory
            // 8000,38:<hex>

            char* comma = strchr(payload, ',');
            if (comma == NULL)
            {
                goto error;
            }
            char* colon = strchr(comma, ':');
            if (colon == NULL)
            {
                goto error;
            }

            uint8_t* mem_offset = (uint8_t*)from_hex_str(payload, comma - payload);
            uint16_t mem_size = from_hex_str(comma + 1, colon - comma - 1);
            from_hex(colon + 1, gdbserver_state.buffer, mem_size * 2);
            memcpy((uint8_t*)mem_offset, gdbserver_state.buffer, mem_size);
            write_ok();
            break;
        }
        case 'Z':
        case 'z':
        {
            // place or delete a breakpoint

            // ignore type and expect comma after type
            payload++;
            if (*payload != ',')
            {
                goto error;
            }
            payload++;

            char* comma = strchr(payload, ',');
            if (comma == NULL)
            {
                goto error;
            }

            uint16_t address = from_hex_str(payload, comma - payload);

            if (command == 'z')
            {
                for (uint8_t i = 0; i < MAX_BREAKPOINTS_COUNT; i++)
                {
                    struct breakpoint_t* b = &gdbserver_state.breakpoints[i];
                    if (b->address != address)
                    {
                        continue;
                    }

                    b->address = 0;
                    // restore original instruction
                    *(uint8_t*)address = b->original_instruction;
                    write_ok();
                    return 1;
                }

                goto error;
            }
            else
            {
                for (uint8_t i = 0; i < MAX_BREAKPOINTS_COUNT; i++)
                {
                    struct breakpoint_t* b = &gdbserver_state.breakpoints[i];
                    if (b->address)
                    {
                        continue;
                    }

                    b->address = address;
                    b->original_instruction = *(uint8_t*)address;
                    *(uint8_t*)address = 0xCF; // RST 08h
                    write_ok();
                    return 1;
                }
            }

            goto error;
        }
        default:
        {
            goto error;
        }
    }

    return 1;
error:
    write_error();
    return 1;
}

uint8_t server_read_data()
{
    char in;
    if (recv(gdbserver_state.client_socket, &in, 1, 0) <= 0)
    {
        return 1;
    }

    switch (in)
    {
        case '$':
        {
            uint8_t read_offset = 0;
            while(1)
            {
                recv(gdbserver_state.client_socket, &in, 1, 0);
                if (in == '#')
                {
                    gdbserver_state.buffer[read_offset] = 0;

                    uint8_t checksum_value = 0;
                    uint8_t sent_checksum = 0;

                    {
                        char checksum[2];
                        recv(gdbserver_state.client_socket, checksum, 2, 0);

                        {
                            uint8_t* buff = gdbserver_state.buffer;

                            for (uint8_t i = 0; i < read_offset; ++i)
                                checksum_value += *buff++;
                        }

                        from_hex(checksum, &sent_checksum, 2);
                    }

                    if (checksum_value == sent_checksum)
                    {
                        return process_packet();
                    }
                    else
                    {
                        write_error();
                    }

                    return 1;
                }
                else
                {
                    gdbserver_state.buffer[read_offset++] = in;
                }
            }
        }
    }

    return 1;
}

