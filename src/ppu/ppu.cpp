#include "../../include/ppu.hpp"
#include <cstdint>
uint8_t PPU::cpu_read(uint16_t addr) {
    switch(addr & 0x0007) {
        case 0x0002: {// $2002 PPUSTATUS
            uint8_t status = (ppu_status_.reg & 0xE0) | (ppu_status_.open_bus_data & 0x1F);

            ppu_status_.vblank = 0;
            w_ = false;

            return status;
        }
        case 0x0004: { // $2004 OAMDATA
            return oam_[oam_addr_];
        }
        case 0x0007: { // $2007 PPUDATA (VRAM Read)
            return ppu_read_buffer_; // [TODO]: placeholder
        }

        default:
        return 0;
    }
}
 
void PPU::cpu_write(uint16_t addr, uint8_t data) {
    switch(addr & 0x0007) {
        case 0x0000: // $2000 PPUCTRL
            ppu_ctrl_.reg = data;
            break;

        case 0x0001: // $2001 PPUMASK
            ppu_mask_.reg = data;
            break;

        case 0x0003: // $2003 OAMADDR
            oam_addr_ = data;
            break;

        case 0x0004: // $2004 OAMDATA
            oam_[oam_addr_] = data;
            oam_addr_++;
            break;
        
        case 0x0005: // $2005 PPUSCROLL
            // [TODO]: placeholder
            break;
        
        case 0x0006: // $2006 PPUADDR
            // [TODO]: placeholder
            break;

        case 0x0007: // $2007 PPUDATA (VRAM Write)
            // [TODO]: placeholder
            break;
    }

}
