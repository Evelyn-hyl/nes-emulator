#include "../../include/mapper.hpp"

MapperZero::MapperZero(uint8_t chr_banks, uint8_t prg_banks) {
  this->chr_banks_ = chr_banks;
  this->prg_banks_ = prg_banks;
};

bool MapperZero::cpu_map_read(uint16_t addr, uint32_t &mapped_addr) {
  if (addr < 0x8000) {
    return false;
  };
  mapped_addr = addr & (prg_banks_ > 1 ? 0x7FFF : 0x3FFF);
  return true;
};

bool MapperZero::cpu_map_write(uint16_t addr, uint32_t &mapped_addr) {
  if (addr < 0x8000) {
    return false;
  };
  mapped_addr = addr & (prg_banks_ > 1 ? 0x7FFF : 0x3FFF);
  return true; // NROM has no writable PRG, but Cartridge decides whether to actually write
};

bool MapperZero::ppu_map_read(uint16_t addr, uint32_t &mapped_addr) {
  if (addr > 0x1FFF) {
    return false;
  };
  mapped_addr = addr; // CHR is fixed, direct passthrough
  return true;
};

bool MapperZero::ppu_map_write(uint16_t addr, uint32_t &mapped_addr) {
  if (addr > 0x1FFF) {
    return false;
  };
  mapped_addr = addr;
  return true; // only meaningful if chrBanks_ == 0 (CHR-RAM)
};
