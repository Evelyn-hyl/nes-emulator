#include "cartridge.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string rom_path = (argc > 1) ? argv[1] : "smb.nes";

    Cartridge cart(rom_path);

    if (!cart.is_valid()) {
        std::cerr << "Cartridge loading failed." << "\n";
        return 1;
    }

    std::cout << "ROM metadata successfully parsed:" << "\n";
    std::cout << "Mapper ID: " << static_cast<int>(cart.get_mapper_id()) << "\n";
    std::cout << "CHR-ROM Size: " << cart.get_chr_size() / 1024 << "KB\n";
    std::cout << "PRG-ROM Size: " << cart.get_prg_size() / 1024 << "KB\n";

    return 0;
}
