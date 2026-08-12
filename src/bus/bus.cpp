#include "../../include/bus.hpp"

unsigned char Bus::read(unsigned short address, Bus::ReadKind kind) {
  switch (kind) {
    case Bus::ReadKind::CPU: {
      if (address <= 0x1FFF) {
        // CPU Onboard Memory Read
        return this->cpu_memory.at(address);
      } else if (address >= 0x8000) {
        // Cartdrige Read CPU_ROM
      }

      break;
    }
    case Bus::ReadKind::PPU: {
      if (address <= 0x1FFF) {
        // Cartdrige Read PPU_ROM
      }

      break;
    }
  }

  return 0;
};
