#ifndef NES_EMULATOR_HPP
#define NES_EMULATOR_HPP

#include "./cpu.hpp"
#include "ppu.hpp"


class NES {
  private:
    CPU cpu;
    PPU ppu;
  
};

#endif
