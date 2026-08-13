#include "ppu.hpp"
#include "cartridge.hpp"

uint8_t PPU::cpu_read(uint16_t addr) {
    uint8_t result = 0;

    switch(addr & 0x0007) {
        case 0x0002: {// $2002 PPUSTATUS
            uint8_t status = (ppu_status_.reg & 0xE0) | (ppu_status_.open_bus_data & 0x1F);

            ppu_status_.vblank = 0;
            w_ = false;

            return result;
        
        case 0x0004: // $2004 OAMDATA
            return oam_[oam_addr_];

        case 0x0007: // $2007 PPUDATA (VRAM Read)
            result = ppu_read_buffer_;
            // [TODO]: placeholder
            return result;
    }

    return open_bus_;
}
 
void PPU::cpu_write(uint16_t addr, uint8_t data) {
    open_bus_ = data;

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

uint8_t PPU::ppu_read(uint16_t addr) {
    addr &= 0x3FFF;

    if (addr <= 0x1FFF) {
        // $0000-$1FFF: CHR-ROM
        return cartridge_ ? cartridge_->ppu_read(addr) : 0x00;
    } else if (addr <= 0x3EFF) {
        // $2000-$3EFF: Internal VRAM
        return vram_[map_vram_addr(addr)];
    } else {
        // [TODO]: Add condition for Palette RAM.
        return 0x00;
    }
}

// Helpers
uint16_t PPU::map_vram_addr(uint16_t addr) const {
    // [TODO]: Complete mapper based on vertical / horizontal scrolling.
    return addr & 0x07FF; // Default: vertical mirroring
}
