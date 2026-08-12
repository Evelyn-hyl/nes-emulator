#ifndef NES_EMULATOR_HPP
#define NES_EMULATOR_HPP

#include "./cpu.hpp"

class NES {
  private:
    Bus bus;
    CPU cpu;

  public:
    NES(){
       this->cpu = CPU(&this->bus); 
    }
};

#endif
