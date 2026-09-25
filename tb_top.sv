`timescale 1ns/1ps

module tb_top (
    input  logic clk,
    input  logic rst_n
);

    logic        req_valid;
    logic        req_we;
    int          req_addr;
    int          req_wdata;

    logic        resp_valid;
    int          resp_rdata;
    logic        irq;

    // DPI-C Imports
    import "DPI-C" function void dpi_bridge_init();
    import "DPI-C" function void dpi_bridge_tick(
        output bit   req_valid,
        output bit   req_we,
        output int   req_addr,
        output int   req_wdata,
        input  bit   resp_valid,
        input  int   resp_rdata,
        input  bit   irq_asserted
    );

    // DUT Instantiation
    dut u_dut (
        .clk        (clk),
        .rst_n      (rst_n),
        .req_valid  (req_valid),
        .req_we     (req_we),
        .req_addr   (req_addr),
        .req_wdata  (req_wdata),
        .resp_valid (resp_valid),
        .resp_rdata (resp_rdata),
        .irq        (irq)
    );

    initial begin
        dpi_bridge_init();
    end

    // Clock-synchronous tick
    always_ff @(posedge clk) begin
        if (rst_n) begin
            dpi_bridge_tick(
                req_valid,
                req_we,
                req_addr,
                req_wdata,
                resp_valid,
                resp_rdata,
                irq
            );
        end
    end

endmodule
