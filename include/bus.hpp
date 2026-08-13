#ifndef NES_BUS_HPP
#define NES_BUS_HPP

#include "cartridge.hpp"
#include <cstdint>


class PPU;
class CPU;
class Cartridge;
class Bus {
	private:
		PPU *ppu_ = nullptr;
		CPU *cpu_ = nullptr;
		Cartridge *cartdridge_ = nullptr;
	public:
		void hook(Cartridge *cartdridge, PPU *ppu, CPU *cpu);
		uint8_t cpu_read_ppu(uint16_t addr);
		void cpu_write_ppu(uint16_t addr, uint8_t data);
};

#endif
