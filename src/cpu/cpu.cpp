#include "../../include/cpu.hpp"

// NOTE In the future it can definitely be possible to
// drop the file size down, as I could probably re use the addressing logic
// with instruction logic.
// For example: Zero Page addressing works the same regardless of where its used
// So we could aim to just re use this core addressing logic, and call it when
// needed. WARNING NOT RIGHT NOW THOUGH AS I WANT TO FOCUS ON GETTING SOMETHING
// FUNCTIONAL BEFORE TIDYING THINGS
CPU::CPU() { this->memory = std::vector<unsigned char>(1024 * 2); };

unsigned char CPU::cpu_read(unsigned short address) const {
  if (address <= 0x1FFF) {
    // CPU Onboard Memory Read
    return this->memory.at(address % 2048);
  } else {
    return this->cartridge_->cpu_read(address);
  };
};

unsigned char CPU::extract(unsigned char data, unsigned char mask,
                           unsigned char shift) const {
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
    this->registers.sr =
        active ? (this->registers.sr | 0x01) : (this->registers.sr & ~0x01);
    break;
  case FlagKind::Z:
    // Set bit mode at 00000010
    this->registers.sr =
        active ? (this->registers.sr | 0x02) : (this->registers.sr & ~0x02);
    break;
  case FlagKind::I:
    // Set bit mode at 00000100
    this->registers.sr =
        active ? (this->registers.sr | 0x04) : (this->registers.sr & ~0x04);
    break;
  case FlagKind::D:
    // Set bit mode at 00001000
    this->registers.sr =
        active ? (this->registers.sr | 0x08) : (this->registers.sr & ~0x08);
    break;
  case FlagKind::B:
    // Set bit mode at 00010000
    this->registers.sr =
        active ? (this->registers.sr | 0x10) : (this->registers.sr & ~0x10);
    break;
  case FlagKind::U:
    // Set bit mode at 00100000
    this->registers.sr =
        active ? (this->registers.sr | 0x20) : (this->registers.sr & ~0x20);
    break;
  case FlagKind::V:
    // Set bit mode at 01000000
    this->registers.sr =
        active ? (this->registers.sr | 0x40) : (this->registers.sr & ~0x40);
    break;
  case FlagKind::N:
    // Set bit mode at 10000000
    this->registers.sr =
        active ? (this->registers.sr | 0x80) : (this->registers.sr & ~0x80);
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
  unsigned char result =
      this->registers.a + memory + this->is_active_flag(FlagKind::C);

  // If it wraps past the max unsigned overflow occurred
  if (result > 0xFF) {
    this->set_flag(FlagKind::C, true);
  };

  // If zero then status is set to true

  if (result == 0x0) {
    this->set_flag(FlagKind::Z, true);
  };

  // If the result's sign is different from both A's and memory's, signed
  // overflow (or underflow) occurred.

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
    this->helper_adc(
        this->cpu_read((zero_page_addr + this->registers.x) % 256));
    break;
  };
  // ADC - Add With Carry Absolute
  // Fetches 16 bit address and loads. High is first, Low is Second
  case 0x6D: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    // Shift the high bits by 8. Then comapre the hangiong low 8 bits, to the
    // low bits retreived from memory.
    unsigned short double_byte_addr = (high << 8) | low;
    this->helper_adc(this->cpu_read(double_byte_addr));
    this->registers.ip += 2;
    break;
  };
  // ADC - Add With Carry Absolute X
  case 0x7D: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    // Shift the high bits by 8. Then comapre the hangiong low 8 bits, to the
    // low bits retreived from memory.
    unsigned short double_byte_addr = (high << 8) | low;
    this->helper_adc(this->cpu_read(double_byte_addr + this->registers.x));
    this->registers.ip += 2;
    break;
  };
  // ADC - Add With Carry Absolute Y
  case 0x79: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    // Shift the high bits by 8. Then comapre the hangiong low 8 bits, to the
    // low bits retreived from memory.
    unsigned short double_byte_addr = (high << 8) | low;
    this->helper_adc(this->cpu_read(double_byte_addr + this->registers.y));
    this->registers.ip += 2;
    break;
  };
  // ADC - Add With Carry Indexed Indirect X (d,x)
  case 0x61: {
    unsigned char arg =
        (this->cpu_read(this->registers.ip) + this->registers.x) % 256;
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
  // AND - Bitwise AND Immediate
  case 0x29: {
    this->registers.a = this->registers.a & this->cpu_read(this->registers.ip);
    this->registers.ip += 1;
    // Set Z flag to equal whether A == 0 or not
    this->set_flag(FlagKind::Z, this->registers.a == 0);
    // Then check set 7th bit of A to the status register N flag
    // So we just isolate the 7th bit, and that will tell us whether 0 or 1
    this->set_flag(FlagKind::N, (this->registers.a & 0x80));
    break;
  };
  // AND - Bitwise AND Zero Page
  case 0x25: {
    unsigned char zero_page_addr = this->cpu_read(this->registers.ip);
    this->registers.a =
        this->registers.a & this->cpu_read(zero_page_addr % 256);
    this->registers.ip += 1;
    this->set_flag(FlagKind::Z, this->registers.a == 0);
    this->set_flag(FlagKind::N, (this->registers.a & 0x80));
    break;
  };
    // AND - Bitwise AND Zero Page X

  case 0x35: {
    unsigned char zero_page_addr = this->cpu_read(this->registers.ip);
    this->registers.a =
        this->registers.a &
        this->cpu_read((zero_page_addr + this->registers.x) % 256);
    this->registers.ip += 1;
    this->set_flag(FlagKind::Z, this->registers.a == 0);
    this->set_flag(FlagKind::N, (this->registers.a & 0x80));
    break;
  };

    // AND - Bitwise AND Absolute

  case 0x2D: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.ip += 2;
    this->registers.a = this->registers.a & this->cpu_read(double_byte_addr);
    this->set_flag(FlagKind::Z, this->registers.a == 0);
    this->set_flag(FlagKind::N, (this->registers.a & 0x80));
    break;
  };

    // AND - Bitwise AND Absolute X

  case 0x3D: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.ip += 2;
    this->registers.a = this->registers.a &
                        this->cpu_read(double_byte_addr + this->registers.x);
    this->set_flag(FlagKind::Z, this->registers.a == 0);
    this->set_flag(FlagKind::N, (this->registers.a & 0x80));
    break;
  };

    // AND - Bitwise AND Absolute Y

  case 0x39: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.ip += 2;
    this->registers.a = this->registers.a &
                        this->cpu_read(double_byte_addr + this->registers.y);
    this->set_flag(FlagKind::Z, this->registers.a == 0);
    this->set_flag(FlagKind::N, (this->registers.a & 0x80));
    break;
  };

    // AND - Bitwise AND (Indirect,X)

  case 0x21: {
    unsigned char arg =
        (this->cpu_read(this->registers.ip) + this->registers.x) % 256;
    this->registers.ip += 1;
    unsigned char low = this->cpu_read(arg);
    unsigned char high = this->cpu_read((arg + 1) % 256);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.a = this->registers.a & this->cpu_read(double_byte_addr);
    this->set_flag(FlagKind::Z, this->registers.a == 0);
    this->set_flag(FlagKind::N, (this->registers.a & 0x80));

    break;
  };

    // AND - Bitwise AND (Indirect),Y

  case 0x31: {
    unsigned char arg = this->cpu_read(this->registers.ip);
    this->registers.ip += 1;
    unsigned char low = this->cpu_read(arg);
    unsigned char high = this->cpu_read((arg + 1) % 256);
    unsigned short double_byte_addr = ((high << 8) | low) + this->registers.y;
    this->registers.a = this->registers.a & this->cpu_read(double_byte_addr);
    this->set_flag(FlagKind::Z, this->registers.a == 0);
    this->set_flag(FlagKind::N, (this->registers.a & 0x80));
    break;
  }
  // BCC - Branch if Carry Clear
  case 0x90: {
    // Signed byte long offset
    char offset = static_cast<char>(this->cpu_read(this->registers.ip));
    // Move past the offset byte
    this->registers.ip += 1;

    if (!this->is_active_flag(FlagKind::C)) {
      this->registers.ip += offset;
    };
    break;
  };

    // BCS - Branch if Carry Set

  case 0xB0: {
    char offset = static_cast<char>(this->cpu_read(this->registers.ip));
    this->registers.ip += 1;

    if (this->is_active_flag(FlagKind::C)) {
      this->registers.ip += offset;
    };

    break;
  };
  // BEQ - Branch if Equal
  // Branches if zero flag is set
  case 0xF0: {
    char offset = static_cast<char>(this->cpu_read(this->registers.ip));
    this->registers.ip += 1;

    if (this->is_active_flag(FlagKind::Z)) {
      this->registers.ip += offset;
    };
    break;
  };

    // BNE - Branch If Not Equal

  case 0xD0: {
    char offset = static_cast<char>(this->cpu_read(this->registers.ip));
    this->registers.ip += 1;

    if (!this->is_active_flag(FlagKind::Z)) {
      this->registers.ip += offset;
    };

    break;
  };

  // BMI - Branch if Minus
  case 0x30: {
    char offset = static_cast<char>(this->cpu_read(this->registers.ip));
    this->registers.ip += 1;

    if (this->is_active_flag(FlagKind::N)) {
      this->registers.ip += offset;
    };
    break;
  };

    // BPL - Branch If Plus

  case 0x10: {
    char offset = static_cast<char>(this->cpu_read(this->registers.ip));
    this->registers.ip += 1;

    if (!this->is_active_flag(FlagKind::N)) {
      this->registers.ip += offset;
    };

    break;
  };

    // BVC - Branch if Overflow Clear

  case 0x50: {
    char offset = static_cast<char>(this->cpu_read(this->registers.ip));
    this->registers.ip += 1;

    if (!this->is_active_flag(FlagKind::V)) {
      this->registers.ip += offset;
    };
    break;
  };

    // BVS - Branch If Overflow Set

  case 0x70: {
    char offset = static_cast<char>(this->cpu_read(this->registers.ip));
    this->registers.ip += 1;

    if (this->is_active_flag(FlagKind::V)) {
      this->registers.ip += offset;
    };
    break;
  };

    // BTT - Bit Test Zero Page

  case 0x24: {
    unsigned char zero_page_addr = this->cpu_read(this->registers.ip);
    unsigned char memory = this->cpu_read(zero_page_addr);
    unsigned char result = this->registers.a & memory;
    this->registers.ip += 1;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::V, (memory & 0x40));
    this->set_flag(FlagKind::N, (memory & 0x80));
    break;
  };

    // BTT - Bit Test Absolute

  case 0x2C: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.ip += 2;
    unsigned char memory = this->cpu_read(double_byte_addr);
    unsigned char result = this->registers.a & memory;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::V, (memory & 0x40));
    this->set_flag(FlagKind::N, (memory & 0x80));
    break;
  };
  // CMP - Compare A Immediate
  case 0xC9: {
    unsigned char memory = this->cpu_read(this->registers.ip);
    this->registers.ip += 1;
    unsigned char result = this->registers.a - memory;
    this->set_flag(FlagKind::C, this->registers.a >= memory);
    this->set_flag(FlagKind::Z, this->registers.a == memory);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  }

    // CMP - Compare A Zero Page

  case 0xC5: {
    unsigned char zero_page_addr = this->cpu_read(this->registers.ip);
    this->registers.ip += 1;
    unsigned char memory = this->cpu_read(zero_page_addr);
    unsigned char result = this->registers.a - memory;
    this->set_flag(FlagKind::C, this->registers.a >= memory);
    this->set_flag(FlagKind::Z, this->registers.a == memory);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

    // CMP - Compare A Zero Page X

  case 0xD5: {
    unsigned char zero_page_addr = this->cpu_read(this->registers.ip);
    this->registers.ip += 1;
    unsigned char memory =
        this->cpu_read((zero_page_addr + this->registers.x) % 256);
    unsigned char result = this->registers.a - memory;
    this->set_flag(FlagKind::C, this->registers.a >= memory);
    this->set_flag(FlagKind::Z, this->registers.a == memory);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };
  // CMP - Compare A Absolute
  case 0xCD: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.ip += 2;
    unsigned char memory = this->cpu_read(double_byte_addr);
    unsigned char result = this->registers.a - memory;
    this->set_flag(FlagKind::C, this->registers.a >= memory);
    this->set_flag(FlagKind::Z, this->registers.a == memory);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

    // CMP - Compare A Absolute X

  case 0xDD: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.ip += 2;
    unsigned char memory = this->cpu_read(double_byte_addr + this->registers.x);
    unsigned char result = this->registers.a - memory;
    this->set_flag(FlagKind::C, this->registers.a >= memory);
    this->set_flag(FlagKind::Z, this->registers.a == memory);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

    // CMP - Compare A Absolute Y

  case 0xD9: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.ip += 2;
    unsigned char memory = this->cpu_read(double_byte_addr + this->registers.y);
    unsigned char result = this->registers.a - memory;
    this->set_flag(FlagKind::C, this->registers.a >= memory);
    this->set_flag(FlagKind::Z, this->registers.a == memory);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

    // CMP - Compare A (Indirect, X)

  case 0xC1: {
    unsigned char arg =
        (this->cpu_read(this->registers.ip) + this->registers.x) % 256;
    this->registers.ip += 1;
    unsigned char low = this->cpu_read(arg);
    unsigned char high = this->cpu_read((arg + 1) % 256);
    unsigned short double_byte_addr = (high << 8) | low;
    unsigned char memory = this->cpu_read(double_byte_addr);
    unsigned char result = this->registers.a - memory;
    this->set_flag(FlagKind::C, this->registers.a >= memory);
    this->set_flag(FlagKind::Z, this->registers.a == memory);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  }
    // CMP - Compare A (Indirect),Y

  case 0xD1: {
    unsigned char arg = this->cpu_read(this->registers.ip);
    this->registers.ip += 1;
    unsigned char low = this->cpu_read(arg);
    unsigned char high = this->cpu_read((arg + 1) % 256);
    unsigned short double_byte_addr = ((high << 8) | low) + this->registers.y;
    unsigned char memory = this->cpu_read(double_byte_addr);
    unsigned char result = this->registers.a - memory;
    this->set_flag(FlagKind::C, this->registers.a >= memory);
    this->set_flag(FlagKind::Z, this->registers.a == memory);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

    // CPX - Compare X Immediate

  case 0xE0: {
    unsigned char memory = this->cpu_read(this->registers.ip);
    this->registers.ip += 1;
    unsigned char result = this->registers.x - memory;
    this->set_flag(FlagKind::C, this->registers.x >= memory);
    this->set_flag(FlagKind::Z, this->registers.x == memory);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

    // CPX - Compare X Zero Page

  case 0xE4: {
    unsigned char zero_page_addr = this->cpu_read(this->registers.ip);
    this->registers.ip += 1;
    unsigned char memory = this->cpu_read(zero_page_addr);
    unsigned char result = this->registers.x - memory;
    this->set_flag(FlagKind::C, this->registers.x >= memory);
    this->set_flag(FlagKind::Z, this->registers.x == memory);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

    // CPX - Compare X Absolute

  case 0xEC: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.ip += 2;
    unsigned char memory = this->cpu_read(double_byte_addr);
    unsigned char result = this->registers.x - memory;
    this->set_flag(FlagKind::C, this->registers.x >= memory);
    this->set_flag(FlagKind::Z, this->registers.x == memory);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

    // CPY - Compare Y Immediate

  case 0xC0: {
    unsigned char memory = this->cpu_read(this->registers.ip);
    this->registers.ip += 1;
    unsigned char result = this->registers.y - memory;
    this->set_flag(FlagKind::C, this->registers.y >= memory);
    this->set_flag(FlagKind::Z, this->registers.y == memory);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };
    // CPY - Compare Y Zero Page

  case 0xC4: {
    unsigned char zero_page_addr = this->cpu_read(this->registers.ip);
    this->registers.ip += 1;
    unsigned char memory = this->cpu_read(zero_page_addr);
    unsigned char result = this->registers.y - memory;
    this->set_flag(FlagKind::C, this->registers.y >= memory);
    this->set_flag(FlagKind::Z, this->registers.y == memory);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

    // CPY - Compare Y Absolute

  case 0xCC: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.ip += 2;
    unsigned char memory = this->cpu_read(double_byte_addr);
    unsigned char result = this->registers.y - memory;
    this->set_flag(FlagKind::C, this->registers.y >= memory);
    this->set_flag(FlagKind::Z, this->registers.y == memory);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

  // DEC - Decrement Memory Zero Page
  case 0xC6: {
    unsigned char zero_page_addr = this->cpu_read(this->registers.ip);
    unsigned char memory = this->cpu_read(zero_page_addr);
    unsigned char result = memory - 1;
    this->memory.at(zero_page_addr) = result;
    this->registers.ip += 1;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

    // DEC - Decrement Memory Zero Page X

  case 0xD6: {
    unsigned char zero_page_addr = this->cpu_read(this->registers.ip);
    unsigned char memory =
        this->cpu_read((zero_page_addr + this->registers.x) % 256);
    unsigned char result = memory - 1;
    this->memory.at((zero_page_addr + this->registers.x) % 256) = result;
    this->registers.ip += 1;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

    // DEC - Decrement Memory Absolute

  case 0xCE: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.ip += 2;
    unsigned char memory = this->cpu_read(double_byte_addr);
    unsigned char result = memory - 1;
    this->memory.at(double_byte_addr) = result;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };
    // DEC - Decrement Memory Absolute X

  case 0xDE: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.ip += 2;
    unsigned char memory = this->cpu_read(double_byte_addr + this->registers.x);
    unsigned char result = memory - 1;
    this->memory.at(double_byte_addr + this->registers.x) = result;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  }

  // DEX - Decrement X Implied
  case 0xCA: {
    unsigned char result = this->registers.x - 1;
    this->registers.x = result;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

  // DEY - Decrement X Implied
  case 0x88: {
    unsigned char result = this->registers.y - 1;
    this->registers.y = result;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

    // EOR - Bitwise Exclusive OR Immediate

  case 0x49: {
    unsigned char memory = this->cpu_read(this->registers.ip);
    this->registers.ip += 1;
    unsigned char result = this->registers.a ^ memory;
    this->registers.a = result;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

    // EOR - Bitwise Exclusive OR Zero Page

  case 0x45: {
    unsigned char zero_page_addr = this->cpu_read(this->registers.ip);
    this->registers.ip += 1;
    unsigned char memory = this->cpu_read(zero_page_addr);
    unsigned char result = this->registers.a ^ memory;
    this->registers.a = result;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  }

    // EOR - Bitwise Exclusive OR Zero Page X

  case 0x55: {
    unsigned char zero_page_addr = this->cpu_read(this->registers.ip);
    unsigned char memory =
        this->cpu_read((zero_page_addr + this->registers.x) % 256);
    this->registers.ip += 1;
    unsigned char result = this->registers.a ^ memory;
    this->registers.a = result;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };
    // EOR - Bitwise Exclusive OR Absolute

  case 0x4D: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.ip += 2;
    unsigned char memory = this->cpu_read(double_byte_addr);
    unsigned char result = this->registers.a ^ memory;
    this->registers.a = result;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  }
    // EOR - Bitwise Exclusive OR Absolute X

  case 0x5D: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.ip += 2;
    unsigned char memory = this->cpu_read(double_byte_addr + this->registers.x);
    unsigned char result = this->registers.a ^ memory;
    this->registers.a = result;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };

    // EOR - Bitwise Exclusive OR Absolute Y

  case 0x59: {
    unsigned char low = this->cpu_read(this->registers.ip);
    unsigned char high = this->cpu_read(this->registers.ip + 1);
    unsigned short double_byte_addr = (high << 8) | low;
    this->registers.ip += 2;
    unsigned char memory = this->cpu_read(double_byte_addr + this->registers.y);
    unsigned char result = this->registers.a ^ memory;
    this->registers.a = result;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  };
    // EOR - Bitwise Exclusive OR (Indirect, X)

  case 0x41: {
    unsigned char arg =
        (this->cpu_read(this->registers.ip) + this->registers.x) % 256;
    this->registers.ip += 1;
    unsigned char low = this->cpu_read(arg);
    unsigned char high = this->cpu_read((arg + 1) % 256);
    unsigned short double_byte_addr = (high << 8) | low;
    unsigned char memory = this->cpu_read(double_byte_addr);
    unsigned char result = this->registers.a ^ memory;
    this->registers.a = result;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  }
    // EOR - Bitwise Exclusive OR (Indirect),Y

  case 0x51: {
    unsigned char arg = this->cpu_read(this->registers.ip);
    this->registers.ip += 1;
    unsigned char low = this->cpu_read(arg);
    unsigned char high = this->cpu_read((arg + 1) % 256);
    unsigned short double_byte_addr = ((high << 8) | low) + this->registers.y;
    unsigned char memory = this->cpu_read(double_byte_addr);
    unsigned char result = this->registers.a ^ memory;
    this->registers.a = result;
    this->set_flag(FlagKind::Z, result == 0);
    this->set_flag(FlagKind::N, (result & 0x80));
    break;
  }

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
