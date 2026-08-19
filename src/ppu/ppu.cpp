#include "ppu.hpp"
#include "cartridge.hpp"

uint8_t PPU::cpu_read(uint16_t addr) {
    uint8_t result = 0;

    switch(addr & 0x0007) {
        case 0x0002: {
            // $2002 PPUSTATUS
            uint8_t status = (ppu_status_.reg & 0xE0) | (open_bus_ & 0x1F);

            ppu_status_.vblank = 0;
            w_ = false;

            return status;
        }
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

void PPU::render_pattern_table(int bank, uint32_t* output_pixel_buffer) {
    // Bank 0 starts at 0x0000, Bank 1 starts at 0x1000
    uint16_t bank_offset = bank * 0x1000;

    constexpr int PATTERN_TABLE_TILES_PER_SIDE = 16;
    constexpr int TILE_PIXELS_PER_SIDE = 8;
    constexpr int PATTERN_TABLE_TOTAL_TILES = PATTERN_TABLE_TILES_PER_SIDE * PATTERN_TABLE_TILES_PER_SIDE;
    constexpr int BITPLANE_SIZE_BYTES = TILE_PIXELS_PER_SIDE;  // one byte per row, one row per pixel-row

    // Divides pattern table by 16-byte tiles (256 tiles total)
    for (int tile_id = 0; tile_id < PATTERN_TABLE_TOTAL_TILES; tile_id++) {
        
        uint16_t tile_offset = bank_offset + (tile_id * 16);

        // Indicates which tile on the 16x16 pattern table grid
        int tile_row = tile_id / PATTERN_TABLE_TILES_PER_SIDE;
        int tile_col = tile_id % PATTERN_TABLE_TILES_PER_SIDE;

        // Divides each tile by 8-pixel(i.e. 2-byte) rows (8 rows total per tile)
        for (int row = 0; row < TILE_PIXELS_PER_SIDE; row++) {

            uint8_t low_bit_plane = ppu_read(tile_offset + row);
            uint8_t high_bit_plane = ppu_read(tile_offset + row + BITPLANE_SIZE_BYTES);

            // Divides each tile row by 8 columns
            for (int col = 0; col < TILE_PIXELS_PER_SIDE; col++) {
                uint8_t low_bit = (low_bit_plane >> (7 - col)) & 0x01;
                uint8_t high_bit = (high_bit_plane >> (7 - col)) & 0x01;
                uint8_t color_index = high_bit << 1 | low_bit;

                // Calculates coordinates on the 128x128 bit grid
                // Each tile row is 8 pixels
                int pixel_row = tile_row * 8 + row;
                int pixel_col = tile_col * 8 + col;

                uint32_t color = 0xFF000000;
                if (color_index == 1) { color = 0xFF555555; };
                if (color_index == 2) { color = 0xFFAAAAAA; };
                if (color_index == 3) { color = 0xFFFFFFFF; };
                
                output_pixel_buffer[pixel_row * NES_WIDTH + pixel_col] = color;
            }
        }
    }
}

// Helpers
uint16_t PPU::map_vram_addr(uint16_t addr) const {
    // [TODO]: Complete mapper based on vertical / horizontal scrolling.
    return addr & 0x07FF; // Default: vertical mirroring
}
