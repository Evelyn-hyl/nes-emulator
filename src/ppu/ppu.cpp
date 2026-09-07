#include "ppu.hpp"

uint8_t PPU::cpu_read(uint16_t addr) {
    uint8_t result = 0;

    switch (addr & 0x0007) {
        case 0x0002: { // $2002 PPUSTATUS
            result = (ppu_status_.reg & 0xE0) | (open_bus_ & 0x1F);

            ppu_status_.vblank = 0;
            w_ = false;

            open_bus_ = result;

            break;
        }
        case 0x0004: { // $2004 OAMDATA
            result = oam_[oam_addr_];

            open_bus_ = result;

            break;
        }
        case 0x0007: { // $2007 PPUDATA (VRAM Read)
            result = ppu_read_buffer_;

            // Fresh vram address read
            ppu_read_buffer_ = ppu_read(v_.reg);

            if (ppu_read_buffer_ >= 0x3F00) {
                result = ppu_read_buffer_;
                ppu_read_buffer_ = ppu_read(v_.reg & 0x2FFF);
            }

            v_.reg += ppu_ctrl_.vram_increment ? 32 : 1;

            open_bus_ = result;

            break;
        }
    }

    return open_bus_;
}

void PPU::cpu_write(uint16_t addr, uint8_t data) {
    open_bus_ = data;

    switch (addr & 0x0007) {
        case 0x0000: // $2000 PPUCTRL
            ppu_ctrl_.reg = data;
            t_.nametable = data & 0x03;
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

        case 0x0005:   // $2005 PPUSCROLL
            if (!w_) { // First write: w == 0
                fine_x_ = data & 0x07;
                t_.coarse_x = data >> 3;
            } else { // Second write: w == 1
                t_.fine_y = data & 0x07;
                t_.coarse_y = data >> 3;
            }

            w_ = !w_;
            break;

        case 0x0006:   // $2006 PPUADDR
            if (!w_) { // First write: w == 0
                t_.reg = t_.reg & 0x00FF | ((data & 0x3F) << 8);
            } else {                             // Second write: w == 1
                t_.reg = t_.reg & 0xFF00 | data; // Intentionally overwrites lower 8 bits
                v_.reg = t_.reg;
            }

            w_ = !w_;
            break;

        case 0x0007: // $2007 PPUDATA (VRAM Write)
            ppu_write(v_.reg, data);

            v_.reg += ppu_ctrl_.vram_increment ? 32 : 1;
            break;
    }
}

uint8_t PPU::ppu_read(uint16_t addr) {
    addr &= 0x3FFF;

    // $0000-$1FFF: CHR-ROM
    if (addr <= 0x1FFF) {
        return cartridge_->ppu_read(addr);
    }
    // $2000-$3EFF: Internal VRAM
    else if (addr <= 0x3EFF) {
        uint16_t vram_index = map_vram_addr(addr, cartridge_->get_mirror_mode());
        return vram_[vram_index];
    }
    // $3F00-$3FFF: Palette RAM
    else {
        // Mask mirroring addresses down to $3F00-$3F1F
        uint16_t pal_ram_index = addr & 0x001F;

        // Mirroring quirk: $3F10, $3F14, $3F18, $3F1C mirror down to $3F00, $3F04, $3F08, $3F0C
        if (pal_ram_index % 4 == 0 && pal_ram_index >= 0x0010) {
            pal_ram_index &= 0x000F;
        }

        return palette_ram_[pal_ram_index];
    }
}

void PPU::ppu_write(uint16_t addr, uint8_t data) {
    // $0000-$1FFF: CHR-RAM
    if (addr <= 0x1FFF) {
        cartridge_->ppu_write(addr, data);
    }
    // $2000-$3EFF: Internal VRAM
    else if (addr <= 0x3EFF) {
        uint16_t vram_index = map_vram_addr(addr, cartridge_->get_mirror_mode());
        vram_[vram_index] = data;
    }
    // $3F00-$3FFF: Palette RAM
    else {
        // Mask mirroring addresses down to $3F00-$3F1F
        uint16_t pal_ram_index = addr & 0x001F;

        // Mirroring quirk: $3F10, $3F14, $3F18, $3F1C mirror down to $3F00, $3F04, $3F08, $3F0C
        if (pal_ram_index % 4 == 0 && pal_ram_index >= 0x0010) {
            pal_ram_index &= 0x000F;
        }

        palette_ram_[pal_ram_index] = data;
    }

    return;
}

void PPU::render_pattern_table(int bank, uint32_t* output_pixel_buffer) {
    // Bank 0 starts at 0x0000, Bank 1 starts at 0x1000
    uint16_t bank_offset = static_cast<uint16_t>(bank) * 0x1000;

    constexpr int PATTERN_TABLE_TILES_PER_SIDE = 16;
    constexpr int TILE_PIXELS_PER_SIDE = 8;
    constexpr int PATTERN_TABLE_TOTAL_TILES = PATTERN_TABLE_TILES_PER_SIDE * PATTERN_TABLE_TILES_PER_SIDE;
    constexpr int BITPLANE_SIZE_BYTES = TILE_PIXELS_PER_SIDE; // one byte per row, one row per pixel-row

    // Divides pattern table by 16-byte tiles (256 tiles total)
    for (int tile_id = 0; tile_id < PATTERN_TABLE_TOTAL_TILES; tile_id++) {

        uint16_t tile_offset = bank_offset + static_cast<uint16_t>(tile_id * 16);

        // Indicates which tile on the 16x16 pattern table grid
        int tile_row = tile_id / PATTERN_TABLE_TILES_PER_SIDE;
        int tile_col = tile_id % PATTERN_TABLE_TILES_PER_SIDE;

        // Divides each tile by 8-pixel(i.e. 2-byte) rows (8 rows total per tile)
        for (int row = 0; row < TILE_PIXELS_PER_SIDE; row++) {

            uint8_t low_bit_plane = ppu_read(tile_offset + static_cast<uint16_t>(row));
            uint8_t high_bit_plane = ppu_read(tile_offset + static_cast<uint16_t>(row + BITPLANE_SIZE_BYTES));

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
                if (color_index == 1) {
                    color = 0xFF555555;
                };
                if (color_index == 2) {
                    color = 0xFFAAAAAA;
                };
                if (color_index == 3) {
                    color = 0xFFFFFFFF;
                };

                output_pixel_buffer[pixel_row * NES_WIDTH + pixel_col] = color;
            }
        }
    }
}

void PPU::clock() {
    bool is_visible_scanline = scanline_ >= 0 && scanline_ <= 239;
    bool is_prerender_scanline = scanline_ == 261;
    bool is_render_line = is_visible_scanline || is_prerender_scanline;

    // Active rendering scanlines and pre-render scanline
    if (is_render_line) {
        if (cycle_ >= 1 && cycle_ <= 256) {
            // Fetch data for current scanline and output to frame buffer
            step();
        }
        if (cycle_ >= 321 && cycle_ <= 336) {
            // [TODO] Fetches data for first two tiles for next scanline
        }
    }

    // VBlank
    if (scanline_ == 241) {
        ppu_ctrl_.nmi_enable = 1;
        // [TODO]
    }

    // Update cycle_ and scanline_ accordingly
}

void PPU::step() {
    // Fine x selects 4 bits, 1 bit from each shift register
    uint8_t bg_pixel = extract_bg_pixel();

    // Lookup Palette RAM & Master Palette
    uint32_t color_pixel = get_bg_pixel_color(bg_pixel);

    // Write to frame buffer
    frame_buffer_[scanline_ * 256 + cycle_] = color_pixel;

    switch (cycle_ % 8) {
        case 0: { // Fetch Pattern Table High Byte
            bg_next_patt_hi_ = bg_next_patt_lo_ | 0x0008;

            // Empty out latches into low byte of the shift registers
            bg_shift_patt_hi_ = bg_shift_patt_hi_ & 0xFF00 | bg_next_patt_hi_;
            bg_shift_patt_lo_ = bg_shift_patt_lo_ & 0xFF00 | bg_next_patt_lo_;

            uint8_t shift_amount = (v_.coarse_y & 0x02) | ((v_.coarse_x & 0x02) >> 1);

            // Bits 0-1 Top-Left, Bits 2-3 Top-Right, Bits 4-5 Bottom-Left, Bits 6-7 Bottom-Right
            uint8_t current_palette = (bg_next_tile_attr_ >> shift_amount * 2) & 0x03;

            bg_shift_attr_hi_ = bg_shift_attr_hi_ & 0xFF00 | ((current_palette & 0x02) >> 1);
            bg_shift_attr_lo_ = bg_shift_attr_lo_ & 0xFF00 | current_palette & 0x01;

            break;
        }
        case 2: // Fetch NameTable Byte
            bg_next_tile_id_ = ppu_read(v_.reg);
            break;

        case 4: { // Fetch Attribute Table Byte
            uint16_t attr_addr =
                0x23C0 | (v_.nametable << 10) | ((v_.coarse_y & 0x001C) << 1) | ((v_.coarse_x & 0x001C) >> 2);
            bg_next_tile_attr_ = ppu_read(attr_addr);
            break;
        }
        case 6: { // Fetch Pattern Table Low Byte
            uint16_t patt_addr = (ppu_ctrl_.bg_pattern << 12) | (bg_next_tile_id_ << 4) | v_.fine_y;
            bg_next_patt_lo_ = ppu_read(patt_addr);
            break;
        }
        default:
            break;
    }

    // Shift registers by 1 bit
    bg_shift_attr_hi_ <<= 1;
    bg_shift_attr_lo_ <<= 1;
    bg_shift_patt_hi_ <<= 1;
    bg_shift_patt_lo_ <<= 1;
}

uint32_t PPU::get_bg_pixel_color(uint8_t bg_pixel) {
    uint8_t palette_ram_offset = (bg_pixel >> 2) * 4 + bg_pixel & 0x0003;

    // [TODO] Replace when adding sprite rendering (starts at $3F10)
    uint32_t nes_color_index = ppu_read(0x3F00 + palette_ram_offset) & 0x3F;

    return SYSTEM_PALETTE[nes_color_index];
}

uint8_t PPU::extract_bg_pixel() {
    uint16_t bit_mux = 0x8000 >> fine_x_;

    uint8_t attr_high = (bg_shift_attr_hi_ & bit_mux) ? 1 : 0;
    uint8_t attr_low = (bg_shift_attr_lo_ & bit_mux) ? 1 : 0;
    uint8_t patt_high = (bg_shift_patt_hi_ & bit_mux) ? 1 : 0;
    uint8_t patt_low = (bg_next_patt_lo_ & bit_mux) ? 1 : 0;

    uint8_t bg_pixel = (attr_high << 3) | (attr_low << 2) | (patt_high << 1) | patt_low;

    return bg_pixel;
}

// Virtual(Nametables) to physical(Banks) address mapper
uint16_t PPU::map_vram_addr(uint16_t addr, Cartridge::MirrorMode mirror_mode) const {
    if (addr >= 0x3000) {
        addr &= 0x2FFF;
    }

    uint16_t offset = addr - 0x2000; // Align with physical array addresses

    // Map address to nametables
    if (mirror_mode == Cartridge::MirrorMode::HORIZONTAL) {
        if (offset <= 0x07FF) {
            // Nametable 0 or 1 offset
            return offset & 0x03FF;
        } else {
            // Nametable 2 or 3 offset
            offset = 0x0400 + (offset & 0x03FF);
            return offset;
        }
    } else { // VERTICAL
        return offset & 0x7FF;
    }
}
