#include "../../include/cpu.hpp"

CPU::CPU() {
  this->memory = std::vector<unsigned char> (1024 * 2);
};

unsigned char CPU::cpu_read(unsigned short address) const {
      if (address <= 0x1FFF) {
        // CPU Onboard Memory Read
        return this->memory.at(address % 2048);
      } else {
        return this->cartridge_->cpu_read(address);
      };
 };

unsigned char CPU::extract(unsigned char data, unsigned char mask, unsigned char shift) const{
  return (data & mask) >> shift;
}

void CPU::set_flag(FlagKind kind, bool active) {
  /*
    Example:
     Sets a bit through or
     e.g 00000000 | 00000001 = 00000001
     Clears a bit through inversion
     e.g 00000001 & ~(00000001) = 00000001 & 11111110 = 00000000
  */

  switch (kind) {
    case FlagKind::C:
      // set bit mode at 00000001
      this->registers.sr = active ? (this->registers.sr | 0x01) : (this->registers.sr & ~0x01);
      break;
    case FlagKind::Z:
      // Set bit mode at 00000010
      this->registers.sr = active ? (this->registers.sr | 0x02) : (this->registers.sr & ~0x02);
      break;
    case FlagKind::I:
      // Set bit mode at 00000100
      this->registers.sr = active ? (this->registers.sr | 0x04) : (this->registers.sr & ~0x04);
      break;
    case FlagKind::D:
      // Set bit mode at 00001000
      this->registers.sr = active ? (this->registers.sr | 0x08) : (this->registers.sr & ~0x08);
      break;
    case FlagKind::B:
      // Set bit mode at 00010000
      this->registers.sr = active ? (this->registers.sr | 0x10) : (this->registers.sr & ~0x10);
      break;
    case FlagKind::U:
      // Set bit mode at 00100000
      this->registers.sr = active ? (this->registers.sr | 0x20) : (this->registers.sr & ~0x20);
      break;
    case FlagKind::V:
      // Set bit mode at 01000000
      this->registers.sr = active ? (this->registers.sr | 0x40) : (this->registers.sr & ~0x40);
      break;
    case FlagKind::N:
      // Set bit mode at 10000000
      this->registers.sr = active ? (this->registers.sr | 0x80) : (this->registers.sr & ~0x80);
      break;
  };
};

bool CPU::is_active_flag(FlagKind kind) {
  switch (kind) {
    case FlagKind::C:
      // Get bit data & 00000001
      return this->extract(this->registers.sr, 0x01);
    case FlagKind::Z:
      // Get bit data & 00000010 >> 1
      return this->extract(this->registers.sr, 0x02, 1);
    case FlagKind::I:
      // Get bit data & 00000100 >> 2
      return this->extract(this->registers.sr, 0x04, 2);
    case FlagKind::D:
      // Get bit data & 00001000 >> 3
      return this->extract(this->registers.sr, 0x08, 3);
    case FlagKind::B:
      // Get bit data & 00010000 >> 4
      return this->extract(this->registers.sr, 0x10, 4);
    case FlagKind::U:
      // Get bit data & 00100000 >> 5
      return this->extract(this->registers.sr, 0x20, 5);
    case FlagKind::V:
      // Get bit data & 01000000 >> 6
      return this->extract(this->registers.sr, 0x40, 6);
    case FlagKind::N:
      // Get bit data & 10000000 >> 7
      return this->extract(this->registers.sr, 0x80, 7);
    default:
      return false;
  };
};

void CPU::helper_adc(unsigned short memory) {
  unsigned char result = this->registers.a + memory + this->is_active_flag(FlagKind::C);

  // If it wraps past the max unsigned overflow occurred
  if (result > 0xFF) {
    this->set_flag(FlagKind::C, true);
  };

  // If zero then status is set to true

  if (result == 0x0) {
    this->set_flag(FlagKind::Z, true);
  };

  // If the result's sign is different from both A's and memory's, signed overflow (or underflow) occurred.

  if (((result ^ this->registers.a) & (result ^ memory) & 0x80) != 0) {
    this->set_flag(FlagKind::V, true);
  }

  // If the 7th bit of the result is on, then negative flag is turned on
  // xxxxxxxx & 0x10000000 != 0
  if ((result & 0x80) != 0) {
    this->set_flag(FlagKind::N, true);
  };

  this->registers.a = result;
}

void CPU::execute() {
  unsigned char op = this->cpu_read(this->registers.ip);
  this->registers.ip += 1;

  switch (op) {
    // ADC - Add With Carry Immediate
    case 0x69: {
      this->helper_adc(this->cpu_read(this->registers.ip));
      this->registers.ip += 1;
      break;
    };
    // ADC - Add With Carry Zero Page
    case 0x65: {
      unsigned char zero_page_addr = this->cpu_read(this->registers.ip);
      this->registers.ip += 1;
      this->helper_adc(this->cpu_read(zero_page_addr % 256));
      break;
    };

      // ADC - Add With Carry Zero Page X

    case 0x75: {
      unsigned char zero_page_addr = this->cpu_read(this->registers.ip);
      this->registers.ip += 1;
      this->helper_adc(this->cpu_read((zero_page_addr + this->registers.x) % 256));
      break;
    };
    // ADC - Add With Carry Absolute
    // Fetches 16 bit address and loads. High is first, Low is Second
    case 0x6D: {
      unsigned char low = this->cpu_read(this->registers.ip);
      unsigned char high = this->cpu_read(this->registers.ip + 1);
      // Shift the high bits by 8. Then comapre the hangiong low 8 bits, to the low bits retreived from memory.
      unsigned short double_byte_addr = (high << 8) | low;
      this->helper_adc(this->cpu_read(double_byte_addr));
      this->registers.ip += 2;
      break;
    };
      // ADC - Add With Carry Absolute X

    case 0x7D: {
      unsigned char low = this->cpu_read(this->registers.ip);
      unsigned char high = this->cpu_read(this->registers.ip + 1);
      // Shift the high bits by 8. Then comapre the hangiong low 8 bits, to the low bits retreived from memory.
      unsigned short double_byte_addr = (high << 8) | low;
      this->helper_adc(this->cpu_read(double_byte_addr + this->registers.x));
      this->registers.ip += 2;
      break;
    };

      // ADC - Add With Carry Absolute Y

    case 0x79: {
      unsigned char low = this->cpu_read(this->registers.ip);
      unsigned char high = this->cpu_read(this->registers.ip + 1);
      // Shift the high bits by 8. Then comapre the hangiong low 8 bits, to the low bits retreived from memory.
      unsigned short double_byte_addr = (high << 8) | low;
      this->helper_adc(this->cpu_read(double_byte_addr + this->registers.y));
      this->registers.ip += 2;
      break;
    };
    // ADC - Add With Carry Indexed Indirect X (d,x)
    case 0x61: {
      unsigned char arg = (this->cpu_read(this->registers.ip) + this->registers.x) % 256;
      this->registers.ip += 1;
      unsigned char low = this->cpu_read(arg);
      unsigned char high = this->cpu_read((arg + 1) % 256);
      unsigned short double_byte_addr = (high << 8) | low;
      this->helper_adc(this->cpu_read(double_byte_addr));
      break;
    };

    // ADC - Add with Carry Indirect Indexed Y (d),y
    case 0x71: {
      unsigned char arg = this->cpu_read(this->registers.ip);
      this->registers.ip += 1;
      unsigned char low = this->cpu_read(arg);
      unsigned char high = this->cpu_read((arg + 1) % 256);
      unsigned short double_byte_addr = ((high << 8) | low) + this->registers.y;
      this->helper_adc(this->cpu_read(double_byte_addr));
      break;
    };
    // SC - Set Carry
    case 0x38: {
      this->set_flag(FlagKind::C, true);
      break;
    };
    // SD - Set Decimal
    case 0xF8: {
      this->set_flag(FlagKind::D, true);
      break;
    };
    // SEI - Set Interrupt Disable
    case 0x78: {
      this->set_flag(FlagKind::I, true);
      break;
    };
    // CLC - Clear Carry
    case 0x18: {
      this->set_flag(FlagKind::C, false);
      break;
    };
    // CLD - Clear Decimal
    case 0xD8: {
      this->set_flag(FlagKind::D, false);
      break;
    };
    // CLI - Clear Interrupt Disable
    case 0x58: {
      this->set_flag(FlagKind::I, false);
      break;
    };
    // CLV - Clear Overflow
    case 0xB8: {
      this->set_flag(FlagKind::V, false);
      break;
    };
  };
};
