#ifndef NES_MAPPER
#define NES_MAPPER

#include <cstdint>

/*
  Mapper interface in case we move beyond mapper 0
*/

class Mapper {
  public:
    virtual ~Mapper() = default;
    virtual bool cpu_map_read(uint16_t addr, uint32_t &mapped_addr) = 0;
    virtual bool cpu_map_write(uint16_t addr, uint32_t &mapped_addr) = 0;
    virtual bool ppu_map_read(uint16_t addr, uint32_t &mapped_addr) = 0;
    virtual bool ppu_map_write(uint16_t addr, uint32_t &mapped_addr) = 0;

  protected:
    uint8_t prg_banks_;
    uint8_t chr_banks_;
};

class MapperZero : public Mapper {

  public:
    MapperZero(uint8_t chr_banks, uint8_t prg_banks);
    virtual bool cpu_map_read(uint16_t addr, uint32_t &mapped_addr) override;
    virtual bool cpu_map_write(uint16_t addr, uint32_t &mapped_addr) override;
    virtual bool ppu_map_read(uint16_t addr, uint32_t &mapped_addr) override;
    virtual bool ppu_map_write(uint16_t addr, uint32_t &mapped_addr) override;
};

#endif
