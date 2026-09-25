#ifndef IPC_PACKET_H
#define IPC_PACKET_H

#include <stdint.h>

#define SOCKET_PATH "/tmp/qemu_rtl.sock"

typedef enum {
    CMD_WRITE     = 0, // Master -> RTL: MMIO Write
    CMD_READ_REQ  = 1, // Master -> RTL: MMIO Read Request
    CMD_READ_RESP = 2, // RTL    -> Master: Read Data Response
    CMD_IRQ       = 3  // RTL    -> Master: Hardware Interrupt Asserted
} cmd_type_t;

typedef struct {
    uint32_t cmd;  // 4 bytes: command type
    uint32_t addr; // 4 bytes: register address offset
    uint32_t data; // 4 bytes: read/write payload
} __attribute__((packed)) rtl_packet_t; // Exactly 12 bytes, zero struct padding

#endif
