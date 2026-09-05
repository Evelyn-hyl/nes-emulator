#pragma once
#include <array>
#include <cstdint>
#include "cartridge.hpp"

class Cartridge;

class PPU {
    public:
    PPU() = default;
    ~PPU() = default;

    static constexpr int NES_WIDTH = 256;
    static constexpr int NES_HEIGHT = 240;

    void set_cartridge(Cartridge *cartridge) { this->cartridge_ = cartridge;};
        
    uint8_t cpu_read(uint16_t addr);
    void cpu_write(uint16_t addr, uint8_t data);
    
    uint8_t ppu_read(uint16_t addr);
    void ppu_write(uint16_t addr, uint8_t data);

    void render_pattern_table(int bank, uint32_t* output_pixel_buffer);

    void clock();
    void reset();

    private:
    /** Internal Hardware Memory Arrays */
    std::array<uint8_t, 2048> vram_{};  // Nametables & Attribute Tables (Bank 0 + Bank 1)
    std::array<uint8_t, 32> palette_ram_{};
    std::array<uint8_t, 256> oam_{};
    Cartridge* cartridge_ = nullptr;    // Access to CHR-ROM for Pattern Tables

    /** Bitfields for $2000 */
    union PPUCTRL {
        uint8_t reg;

        struct {
            uint8_t nametable_x     : 1;    // 0 = $2000, 1 = $2400, 2 = $2800, 3 = $2C00
            uint8_t nametable_y     : 1;
            uint8_t vram_increment  : 1;    // 0: add 1, 1: add 32
            uint8_t sprite_pattern  : 1;    // 0: $0000, 1: $1000 (ignored in 8x16 mode)
            uint8_t bg_pattern      : 1;    // 0: $0000, 1: $1000
            uint8_t sprite_size     : 1;    // 0: 8x8 pixels, 1: 8x16 pixels
            uint8_t master_slave    : 1;    // Unused for NES games
            uint8_t nmi_enable      : 1;    // 0: off, 1: on
        };
    };

    /** Bitfields for $2001 */
    union PPUMASK {
        uint8_t reg;

        struct {
            uint8_t greyscale         : 1;
            uint8_t show_bg_left      : 1;
            uint8_t show_sprite_left  : 1;
            uint8_t render_bg         : 1;
            uint8_t render_sprites    : 1;
            uint8_t emphasize_red     : 1;
            uint8_t emphasize_green   : 1;
            uint8_t emphasize_blue    : 1;
        };
    };

    /** Bitfields for $2002 */
    union PPUSTATUS {
        uint8_t reg;

        struct {
            uint8_t padding          : 5;
            uint8_t sprite_overflow  : 1;
            uint8_t sprite_0_hit     : 1;
            uint8_t vblank           : 1;
        };
    };

    /** Bitfields for $v and $t */
    union LoopyRegister {
        uint16_t reg;

        struct {
            uint16_t coarse_x   : 5;    // bits 0-4
            uint16_t coarse_y   : 5;    // bits 5-9
            uint16_t nametable  : 2;    // bits 10-11
            uint16_t fine_y     : 3;    // bits 12-14
            uint16_t unused     : 1;
        };
    };

    /** Registers */
    PPUCTRL ppu_ctrl_{};      // $2000
    PPUMASK ppu_mask_{};      // $2001
    PPUSTATUS ppu_status_{};  // $2002
    uint8_t oam_addr_{};      // $2003

    /** Internal Buffers */
    uint8_t ppu_read_buffer_{};
    uint8_t open_bus_{};

    /** Scroll & Address Pointers ($2005 - $2006) */
    LoopyRegister v_{};     // During rendering: scroll position | Outside of rendering: vram address pointer
    LoopyRegister t_{};     // During rendering: coarse-x scroll for next scanline & starting y-scroll | Outside of rendering: temp vram address pointer
    uint16_t fine_x_{};     // Horizontal pixel offset
    bool w_{};              // Write latch        

    /** Helpers */  
    uint16_t map_vram_addr(uint16_t addr, Cartridge::MirrorMode mirror_mode) const;
};