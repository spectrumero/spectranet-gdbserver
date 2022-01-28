#include "server.h"
#include "sys/socket.h"
#include "sockpoll.h"
#include "utils.h"
#include "state.h"
#include "spectranet.h"

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

static void write_packet_bytes(const uint8_t *data, uint8_t num_bytes)
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

static void write_packet(const char *data)
{
    write_packet_bytes((const uint8_t *)data, strlen(data));
}

uint8_t server_listen()
{
    if (gdbserver_state.client_socket)
    {
        print42("execution stopped\n");
        // report we have trapped
        write_packet("T05thread:p01.01;");
        return 0;
    }

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

static void server_on_disconnect()
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
    {"C",                       "QCp01x.01x"},
    {"Attached",                "1"},
    {"Offsets",                 ""},
    {"Supported",               "PacketSize=128;qXfer:features:read+;qXfer:auxv:read+"},
    {"Symbol",                  "OK"},
    {"TStatus",                 ""},
    {"fThreadInfo",             "mp01.01"},
    {"sThreadInfo",             "l"},
    {NULL,                      NULL},
};

static void process_query(char *payload) __z88dk_callee
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
            "<reg name=\"af'\" bitsize=\"16\" type=\"int\"/>"
            "</feature><architecture>z80</architecture></target>#81");
        return;
    }
    for (uint8_t i = 0; queries[i].request; i++)
    {
        if (strcmp(queries[i].request, payload) == 0)
        {
            write_packet(queries[i].response);
            return;
        }
    }

    write_packet("");
}

static void write_error() __z88dk_callee
{
    write_packet("E01");
}

static void write_ok() __z88dk_callee
{
    write_packet("OK");
}

#define NMISTACK (0x38FE)

static uint8_t process_packet(char* payload) __z88dk_callee
{
    char command = *payload++;
    switch (command)
    {
        case 'c':
        {
            // continue execution
            print42("resuming execution\n");
            return 0;
        }
        case 'q':
        {
            process_query(payload);
            return 1;
        }
        case '?':
        {
            // we're always stopped when we're under execution
            write_packet("T05thread:p01.01;");
            return 1;
        }
        case 'g':
        {
            uint16_t regs[7];

            // sp
            uint16_t sp = *(uint16_t*)NMISTACK;
            regs[0] = sp + 2;
            // pc
            regs[1] = *(uint16_t*)(sp);
            // hl
            regs[2] = *(uint16_t*)(NMISTACK - 6);
            // de
            regs[3] = *(uint16_t*)(NMISTACK - 8);
            // bc
            regs[4] = *(uint16_t*)(NMISTACK - 10);
            // af
            regs[5] = *(uint16_t*)(NMISTACK - 12);
            // afalt
            regs[6] = *(uint16_t*)(NMISTACK - 14);

            // dump registers
            to_hex(regs, gdbserver_state.buffer, 14);
            write_packet_bytes(gdbserver_state.buffer, 28);
            return 1;
        }
        case 'G':
        {
            // set registers

            uint16_t regs[7];
            from_hex(payload, regs, 28);

            // sp
            uint16_t sp = regs[0];
            sp -= 2;
            // pc
            *(uint16_t*)(sp) = regs[1];
            *(uint16_t*)NMISTACK = sp;
            // hl
            *(uint16_t*)(NMISTACK - 6) = regs[2];
            // de
            *(uint16_t*)(NMISTACK - 8) = regs[3];
            // bc
            *(uint16_t*)(NMISTACK - 10) = regs[4];
            // af
            *(uint16_t*)(NMISTACK - 12) = regs[5];
            // afalt
            *(uint16_t*)(NMISTACK - 14) = regs[6];

            write_ok();
            return 1;
        }
        case 'm':
        {
            // read memory
            char* comma = strchr(payload, ',');
            if (comma == NULL)
            {
                write_error();
            }
            else
            {
                uint8_t* mem_offset = (uint8_t*)from_hex_str(payload, comma - payload);
                comma++;
                uint16_t mem_size = from_hex_str(comma, strlen(comma));
                to_hex(mem_offset, gdbserver_state.buffer, mem_size);
                write_packet_bytes(gdbserver_state.buffer, mem_size * 2);
            }
            return 1;
        }
        case 'M':
        {
            // write memory
            char* colon = strchr(payload, ':');
            char* comma = strchr(payload, ',');
            if (comma == NULL || colon == NULL)
            {
                write_error();
            }
            else
            {
                uint8_t* mem_offset = (uint8_t*)from_hex_str(payload, comma - payload);
                comma++;
                colon++;
                uint16_t mem_size = from_hex_str(comma, colon - comma);
                from_hex(colon, gdbserver_state.buffer, mem_size);
                memcpy((uint8_t*)mem_offset, gdbserver_state.buffer, mem_size);
                write_ok();
            }
            return 1;
        }
    }

    return 1;
}

static uint8_t server_read_data()
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
                        return process_packet((char*)gdbserver_state.buffer);
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

uint8_t server_iteration()
{
    switch (poll_fd(gdbserver_state.client_socket))
    {
        case POLLHUP:
        {
            server_on_disconnect();
            return 1;
        }
        case POLLIN:
        {
            return server_read_data();
        }
        default:
        {
            return 1;
        }
    }
}
