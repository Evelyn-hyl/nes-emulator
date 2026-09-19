#pragma once

#include <cstdint>

class PPU;
class CPU;
class Cartridge;

class Bus {
    public:
    void hook(Cartridge* cartridge, PPU* ppu, CPU* cpu);
    uint8_t cpu_read_ppu(uint16_t addr);
    void cpu_write_ppu(uint16_t addr, uint8_t data);

    private:
    PPU* ppu_ = nullptr;
    CPU* cpu_ = nullptr;
    Cartridge* cartridge_ = nullptr;
};
