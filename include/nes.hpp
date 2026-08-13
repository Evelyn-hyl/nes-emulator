#ifndef NES_EMULATOR_HPP
#define NES_EMULATOR_HPP

#include "./cpu.hpp"
#include "ppu.hpp"

class NES {
  private:
    Bus bus;
    CPU cpu;
    PPU ppu;
  public:
    NES(){
      this->bus.hook(&this->ppu, &this->cpu);
    }
};

#endif
