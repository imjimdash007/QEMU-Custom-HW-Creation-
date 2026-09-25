`timescale 1ns/1ps

module dut (
    input  logic        clk,
    input  logic        rst_n,

    // Down path: Transactions coming from Host (QEMU / Test script)
    input  logic        req_valid,
    input  logic        req_we,        // 1 = Write, 0 = Read
    input  logic [31:0] req_addr,
    input  logic [31:0] req_wdata,

    // Up path: Response returning to Host
    output logic        resp_valid,
    output logic [31:0] resp_rdata,

    // Up path: Asynchronous Event (Interrupt)
    output logic        irq
);

    // Register Map:
    // 0x00: Scratchpad register (Read/Write, default: 0xA5A50000)
    // 0x04: Control register    (Write bit 0 = 1 to pulse IRQ)
    logic [31:0] reg_scratch;
    logic        irq_pulse;

    assign irq = irq_pulse;

    // Pure synchronous sequential block
    always_ff @(posedge clk) begin
        if (!rst_n) begin
            reg_scratch <= 32'hA5A50000;
            resp_valid  <= 1'b0;
            resp_rdata  <= 32'h0;
            irq_pulse   <= 1'b0;
        end else begin
            resp_valid <= 1'b0;
            irq_pulse  <= 1'b0;

            if (req_valid) begin
                if (req_we) begin
                    // Write transaction
                    case (req_addr)
                        32'h00: reg_scratch <= req_wdata;
                        32'h04: irq_pulse   <= req_wdata[0];
                        default: ; 
                    endcase
                end else begin
                    // Read transaction (1-cycle synchronous response)
                    resp_valid <= 1'b1;
                    case (req_addr)
                        32'h00:  resp_rdata <= reg_scratch;
                        default: resp_rdata <= 32'hDEADBEEF;
                    endcase
                end
            end
        end
    end

endmodule
