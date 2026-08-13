#include "../../include/bus.hpp"
#include "../../include/cpu.hpp"
#include "../../include/ppu.hpp"

void Bus::hook(Cartridge *cartdridge, PPU *ppu, CPU *cpu) {
  this->cpu_->set_cartdridge(cartdridge);
  this->ppu_->set_cartdridge(cartdridge);
};

uint8_t Bus::cpu_read_ppu(uint16_t addr) {
  if (this->ppu_ != nullptr) {
    return this->ppu_->cpu_read(addr);
  }
  return 0;
};

void Bus::cpu_write_ppu(uint16_t addr, uint8_t data) {
  if (this->ppu_ != nullptr) {
    this->ppu_->cpu_write(addr, data);
  }
};
