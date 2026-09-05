#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "mapper.hpp"
class Cartridge {
    public:
    enum MirrorMode {
        VERTICAL,
        HORIZONTAL
    };

    explicit Cartridge(const std::string& filename);

    bool is_valid() const { return valid_; };

    uint8_t get_mapper_id() const { return mapper_id_; };
    size_t get_prg_size() const { return prg_rom_.size(); };
    size_t get_chr_size() const { return chr_rom_.size(); };
    MirrorMode get_mirror_mode() const { return mirror_mode_; };

    void set_mirror_mode(MirrorMode mirror_mode) { mirror_mode_ = mirror_mode; };

    uint8_t ppu_read(uint16_t addr) const;
    uint8_t cpu_read(uint16_t addr) const;
    void ppu_write(uint16_t addr, uint8_t data);

    private:
    struct iNESHeader {
        char name[4];
        uint8_t prg_rom_chunks; // PRG-ROM size in 16KB units
        uint8_t chr_rom_chunks; // CHR-ROM size in 8KB units
        uint8_t mapper_id_low;
        uint8_t mapper_id_high;
        uint8_t padding[8];
    };
    
    bool valid_{};
    uint8_t mapper_id_{};
    MirrorMode mirror_mode_{};
    Mapper *mapper_;
    std::vector<uint8_t> prg_rom_;
    std::vector<uint8_t> chr_rom_;
};
