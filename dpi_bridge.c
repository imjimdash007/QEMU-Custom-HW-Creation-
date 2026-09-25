#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "svdpi.h"
#include "ipc_packet.h"

static int server_fd = -1;
static int client_fd = -1;

// ----------------------------------------------------------------------
// dpi_bridge_init: Called once in the SystemVerilog initial block
// ----------------------------------------------------------------------
void dpi_bridge_init(void) {
    struct sockaddr_un addr;

    unlink(SOCKET_PATH);
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[DPI-C] Socket creation failed");
        return;
    }

    // Put listening socket in non-blocking mode
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[DPI-C] Bind failed");
        close(server_fd);
        server_fd = -1;
        return;
    }

    if (listen(server_fd, 1) < 0) {
        perror("[DPI-C] Listen failed");
        close(server_fd);
        server_fd = -1;
        return;
    }

    printf("[DPI-C] Bridge listening on %s\n", SOCKET_PATH);
}

// ----------------------------------------------------------------------
// dpi_bridge_tick: Called every posedge clk by the SystemVerilog harness
// ----------------------------------------------------------------------
void dpi_bridge_tick(
    // Down path: Host -> RTL
    svBit*      req_valid,
    svBit*      req_we,
    int*        req_addr,
    int*        req_wdata,

    // Up path: RTL -> Host (read data responses)
    const svBit resp_valid,
    const int   resp_rdata,

    // Up path: RTL -> Host (hardware interrupts)
    const svBit irq_asserted
) {
    *req_valid = 0;

    // 1. If no client connected, try to accept one non-blockingly
    if (client_fd < 0 && server_fd >= 0) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd >= 0) {
            int flags = fcntl(client_fd, F_GETFL, 0);
            fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
            printf("[DPI-C] Client connected to socket!\n");
        }
    }

    if (client_fd < 0) return;

    // 2. UP PATH: Deliver synchronous read responses back to client
    if (resp_valid) {
        rtl_packet_t resp;
        resp.cmd  = CMD_READ_RESP;
        resp.addr = 0;
        resp.data = (uint32_t)resp_rdata;
        if (write(client_fd, &resp, sizeof(resp)) < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                close(client_fd);
                client_fd = -1;
                return;
            }
        }
    }

    // 3. UP PATH: Deliver asynchronous hardware interrupts to client
    if (irq_asserted) {
        rtl_packet_t irq_pkt;
        irq_pkt.cmd  = CMD_IRQ;
        irq_pkt.addr = 0;
        irq_pkt.data = 1;
        if (write(client_fd, &irq_pkt, sizeof(irq_pkt)) < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                close(client_fd);
                client_fd = -1;
                return;
            }
        }
    }

    // 4. DOWN PATH: Poll for requests from client (read/write commands)
    rtl_packet_t in_pkt;
    ssize_t bytes = read(client_fd, &in_pkt, sizeof(in_pkt));

    if (bytes == (ssize_t)sizeof(in_pkt)) {
        *req_valid = 1;
        *req_we    = (in_pkt.cmd == CMD_WRITE) ? 1 : 0;
        *req_addr  = (int)in_pkt.addr;
        *req_wdata = (int)in_pkt.data;
    } else if (bytes == 0) {
        printf("[DPI-C] Client disconnected.\n");
        close(client_fd);
        client_fd = -1;
    } else {
        // bytes < 0: No data available (EAGAIN/EWOULDBLOCK) is standard behavior
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            close(client_fd);
            client_fd = -1;
        }
    }
}
