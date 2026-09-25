// sim_main.cpp - Verilator execution harness
#include <memory>
#include <unistd.h>
#include "Vtb_top.h"
#include "verilated.h"

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    auto top = std::make_unique<Vtb_top>();

    // Step 1: Hold reset active-low (rst_n = 0)
    top->clk = 0;
    top->rst_n = 0;

    for (int i = 0; i < 20; ++i) {
        top->clk = !top->clk;
        top->eval();
    }

    // Step 2: Release reset
    top->rst_n = 1;

    // Step 3: Clock oscillation loop
    while (!Verilated::gotFinish()) {
        top->clk = !top->clk;
        top->eval();

        // Brief yield prevents 100% host CPU thrashing while idling
        static uint64_t ticks = 0;
        if ((++ticks % 20000) == 0) {
            usleep(200);
        }
    }

    return 0;
}
