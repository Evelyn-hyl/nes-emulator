#include "bus.hpp"
#include "cpu.hpp"
#include "ppu.hpp"

void Bus::hook(Cartridge* cartridge, PPU* ppu, CPU* cpu) {
    cartridge_ = cartridge;
    cpu_ = cpu;
    ppu_ = ppu;

    cpu_->set_cartridge(cartridge_);
    ppu_->set_cartridge(cartridge_);
};

uint8_t Bus::cpu_read_ppu(uint16_t addr) {
    if (ppu_ != nullptr) {
        return ppu_->cpu_read(addr);
    }
    return 0;
};

void Bus::cpu_write_ppu(uint16_t addr, uint8_t data) {
    if (ppu_ != nullptr) {
        ppu_->cpu_write(addr, data);
    }
};
