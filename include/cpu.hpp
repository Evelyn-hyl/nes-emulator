#ifndef NES_CPU_HPP
#define NES_CPU_HPP

#include "./bus.hpp"
#include "./cartridge.hpp"
struct Registers {
    // 16 bit program counter or instruction pointer
    unsigned short ip;
    // 256 byte stack pointer register
    unsigned char sp;
    // 8 bit status register (Only 6 bits are used)
    unsigned char sr;
    // 8 bit wide accumulator
    unsigned char a;
    // 8 bit index X
    unsigned char x;
    // 8 bit index Y
    unsigned char y;

    Registers() {
      this->a = 0;
      this->x = 0;
      this->y = 0;
      this->ip = 0;
      this->sr = 0;
      this->sp = 0xFF;
    }
};

enum FlagKind {
  // Carry Bit 0 - If Result > 0xFF, unsigned overflow occurred
  // If Result < 0x00, unsigned underflow occurred
  C,
  // Zero Bit 1 - Checks if result or data loaded == 0x00
  Z,
  // Interrupt Bit 2 - When on, hardware interrupts are blocked and ignored by
  // the CPU
  I,
  // Decimal Bit 3 - Tells us whether decimal mode is active or not
  D,
  // Break Bit 4 - Tells us whether an interrupt was caused by a software BRK vs
  // hardware
  B,
  // Unused Bit 5 - Always constant and ignored in hardware behaviour
  U,
  // Overflow Bit 6 - Set during arithmetic when an underflow or overflow breaks
  // two complement bounds.
  V,
  // Negative Bit 7 - Set if the highest bit of the result is set, representing
  // a negative number in signed binary
  N
};

class CPU {
  public:
    CPU();
    void execute();

    unsigned char extract(unsigned char data, unsigned char mask, unsigned char shift = 0) const;
    /*
      Turns on selected flag in the status register (sr)
    */
    void set_flag(FlagKind kind, bool active);
    /*
      Checks if flag in the status register (sr) is active
    */
    bool is_active_flag(FlagKind kind);
    void helper_adc(unsigned short memory);
    unsigned char cpu_read(unsigned short address) const;
    void set_cartdridge(Cartridge *cartdridge) {
      this->cartridge_ = cartdridge;
    };

  private:
    Cartridge *cartridge_ = nullptr; // Access to PRG-ROM
    // 2KB onboard memory from [0x0000, 0x07FF]
    std::vector<unsigned char> memory;
    Registers registers;
    Bus *bus;
};

#endif
