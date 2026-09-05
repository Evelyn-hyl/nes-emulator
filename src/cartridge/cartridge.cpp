#include "../../include/cartridge.hpp"
#include <fstream>
#include <iostream>

namespace {
    constexpr size_t prg_rom_chunk_size = 16384;
    constexpr size_t chr_rom_chunk_size = 8192;
} // namespace

uint8_t Cartridge::cpu_read(uint16_t address) const {
    uint32_t mapped;

    if (mapper_->cpu_map_read(address, mapped)) {
        return prg_rom_.at(mapped);
    };

    std::cerr << "[CARTRIDGE][CPU_READ ERROR] Performed read on address outside of prg_rom range!" << std::endl;

    return 0;
};

uint8_t Cartridge::ppu_read(uint16_t addr) const {
    uint32_t mapped;

    if (mapper_ && mapper_->ppu_map_read(addr, mapped)) {
        return chr_rom_[mapped];
    };

    std::cerr << "[CARTRIDGE][PPU_READ ERROR] Performed read on address outside of chr_rom / chr_ram range!" << std::endl;

    return 0x00;
};

void Cartridge::ppu_write(uint16_t addr, uint8_t data) {
    uint32_t mapped;

    if (mapper_ && mapper_->ppu_map_write(addr, mapped)) {
        chr_rom_[mapped] = data;
        return;
    }

    std::cerr << "[CARTRIDGE][PPU_WRITE ERROR] Performed write on address outside of chr_ram range!" << "\n";

    return;
}

Cartridge::Cartridge(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed to open ROM file: " << filename << "\n";
        return;
    }

    iNESHeader header;
    file.read(reinterpret_cast<char *>(&header), sizeof(iNESHeader));

    if (header.name[0] != 'N' || header.name[1] != 'E' || header.name[2] != 'S' || header.name[3] != 0x1A) {
        std::cerr << "Invalid iNES header." << "\n";
        return;
    }

    mapper_id_ = (header.mapper_id_high & 0xF0) | (header.mapper_id_low >> 4);

    set_mirror_mode((header.mapper_id_low & 0x01) ? VERTICAL : HORIZONTAL);

    prg_rom_.resize(header.prg_rom_chunks * prg_rom_chunk_size);
    file.read(reinterpret_cast<char *>(prg_rom_.data()), prg_rom_.size());

    if (header.chr_rom_chunks > 0) {
        chr_rom_.resize(header.chr_rom_chunks * chr_rom_chunk_size);
        file.read(reinterpret_cast<char *>(chr_rom_.data()), chr_rom_.size());
    } else {    // Program uses CHR-RAM
        chr_rom_.resize(chr_rom_chunk_size, 0);
    }

    uint8_t prg_banks = header.prg_rom_chunks;
    uint8_t chr_banks = header.chr_rom_chunks;

    switch (mapper_id_) {
        case 0: {
            this->mapper_ = new MapperZero(chr_banks, prg_banks);
            break;
        }
    }

    valid_ = true;
}
