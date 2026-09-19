#ifndef NES_EMULATOR_HPP
#define NES_EMULATOR_HPP

#include "./cpu.hpp"
#include "ppu.hpp"

class NES {
    public:
    NES() { bus_.hook(&ppu_, &cpu_); }

    uint64_t get_global_cpu_cycles() { return global_cpu_cycles_; }
    void set_global_cpu_cycles(int cycles_to_add) { global_cpu_cycles_ += cycles_to_add; }

    private:
    Bus bus_;
    CPU cpu_;
    PPU ppu_;
    uint64_t global_cpu_cycles_;
};

#endif
