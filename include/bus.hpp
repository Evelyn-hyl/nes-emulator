#ifndef NES_BUS_HPP
#define NES_BUS_HPP

#include <vector>

class Bus {
  private:
    // CPU has 2KB (2 * 1024) of onboard memory, everything else is managed by mapper and bus. Since memory must be accessed
    // by other components We will leave all on the bus.
    std::vector<unsigned char> cpu_memory = std::vector<char unsigned>(2 * 1024u);
    ;

  public:
    enum ReadKind {
      CPU,
      PPU
    };
    unsigned char read(unsigned short address, ReadKind kind = Bus::ReadKind::CPU);
};

#endif
