#include "cpu.hpp"
#include "cartridge.hpp"

uint8_t CPU::cpu_read(uint16_t address) const {
    if (address <= 0x1FFF) {
        // CPU Onboard Memory Read
        return memory_.at(address % 2048);
    } else {
        return cartridge_->cpu_read(address);
    }
}

void CPU::cpu_write(uint16_t address, uint8_t value) {
    if (address <= 0x1FFF) {
        memory_.at(address % memory_.size()) = value;
        return;
    }

    // Route other address ranges through the bus/cartridge.
}

uint8_t CPU::stack_pop() {
    registers_.sp++;
    return cpu_read(0x0100 + registers_.sp);
}

void CPU::stack_push(uint8_t value) {
    cpu_write(0x0100 + registers_.sp, value);
    registers_.sp--;

    return;
}

uint8_t CPU::extract(uint8_t data, uint8_t mask, uint8_t shift) const { return (data & mask) >> shift; }

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
            registers_.sr = active ? (registers_.sr | 0x01) : (registers_.sr & ~0x01);
            break;
        case FlagKind::Z:
            // Set bit mode at 00000010
            registers_.sr = active ? (registers_.sr | 0x02) : (registers_.sr & ~0x02);
            break;
        case FlagKind::I:
            // Set bit mode at 00000100
            registers_.sr = active ? (registers_.sr | 0x04) : (registers_.sr & ~0x04);
            break;
        case FlagKind::D:
            // Set bit mode at 00001000
            registers_.sr = active ? (registers_.sr | 0x08) : (registers_.sr & ~0x08);
            break;
        case FlagKind::B:
            // Set bit mode at 00010000
            registers_.sr = active ? (registers_.sr | 0x10) : (registers_.sr & ~0x10);
            break;
        case FlagKind::U:
            // Set bit mode at 00100000
            registers_.sr = active ? (registers_.sr | 0x20) : (registers_.sr & ~0x20);
            break;
        case FlagKind::V:
            // Set bit mode at 01000000
            registers_.sr = active ? (registers_.sr | 0x40) : (registers_.sr & ~0x40);
            break;
        case FlagKind::N:
            // Set bit mode at 10000000
            registers_.sr = active ? (registers_.sr | 0x80) : (registers_.sr & ~0x80);
            break;
    }
}

bool CPU::is_flag_active(FlagKind kind) {
    switch (kind) {
        case FlagKind::C:
            // Get bit data & 00000001
            return extract(registers_.sr, 0x01);
        case FlagKind::Z:
            // Get bit data & 00000010 >> 1
            return extract(registers_.sr, 0x02, 1);
        case FlagKind::I:
            // Get bit data & 00000100 >> 2
            return extract(registers_.sr, 0x04, 2);
        case FlagKind::D:
            // Get bit data & 00001000 >> 3
            return extract(registers_.sr, 0x08, 3);
        case FlagKind::B:
            // Get bit data & 00010000 >> 4
            return extract(registers_.sr, 0x10, 4);
        case FlagKind::U:
            // Get bit data & 00100000 >> 5
            return extract(registers_.sr, 0x20, 5);
        case FlagKind::V:
            // Get bit data & 01000000 >> 6
            return extract(registers_.sr, 0x40, 6);
        case FlagKind::N:
            // Get bit data & 10000000 >> 7
            return extract(registers_.sr, 0x80, 7);
        default:
            return false;
    }
}

void CPU::add_with_carry(uint16_t operand) {
    uint8_t result = static_cast<uint8_t>(registers_.a + operand + is_flag_active(FlagKind::C));

    // If it wraps past the max unsigned overflow occurred
    if (result > 0xFF) {
        set_flag(FlagKind::C, true);
    }

    // If zero then status is set to true
    if (result == 0x0) {
        set_flag(FlagKind::Z, true);
    }

    // If the result's sign is different from both A's and operand's, signed
    // overflow (or underflow) occurred.
    if (((result ^ registers_.a) & (result ^ operand) & 0x80) != 0) {
        set_flag(FlagKind::V, true);
    }

    // If the 7th bit of the result is on, then negative flag is turned on
    // xxxxxxxx & 0x10000000 != 0
    if ((result & 0x80) != 0) {
        set_flag(FlagKind::N, true);
    }

    registers_.a = result;
}

void CPU::set_zn_flags(uint8_t reg) {
    set_flag(FlagKind::Z, reg == 0);
    set_flag(FlagKind::N, (reg & 0x80));
}

void CPU::compare(uint8_t reg, uint8_t operand) {
    uint8_t result = reg - operand;

    set_flag(FlagKind::C, reg >= operand);
    set_flag(FlagKind::Z, reg == operand);
    set_flag(FlagKind::N, (result & 0x80));
}

CPU::AddressResult CPU::get_indexed_indirect_x_addr() {
    uint8_t operand = cpu_read(registers_.ip++);

    // Truncate to 8 bits and wrap within zero page
    uint8_t pointer = (operand + registers_.x) % 256;

    uint8_t low = cpu_read(pointer);
    uint8_t high = cpu_read((pointer + 1) % 256);

    uint16_t address = (static_cast<uint16_t>(high) << 8) | low;
    return {address, false};
}

CPU::AddressResult CPU::get_indirect_indexed_y_addr() {
    uint8_t pointer = cpu_read(registers_.ip++);

    uint8_t low = cpu_read(pointer);
    uint8_t high = cpu_read((pointer + 1) % 256);

    uint16_t base_address = (static_cast<uint16_t>(high) << 8) | low;

    uint16_t address = static_cast<uint16_t>(base_address + registers_.y);
    bool is_page_crossed = (base_address & 0xFF00) != (address & 0xFF00);

    return {address, is_page_crossed};
}

uint16_t CPU::get_absolute_address() {
    uint8_t low = cpu_read(registers_.ip++);
    uint8_t high = cpu_read(registers_.ip++);

    return (static_cast<uint16_t>(high) << 8) | low;
}

CPU::AddressResult CPU::get_absolute_x_addr() {
    uint16_t base_address = get_absolute_address();
    uint16_t address = static_cast<uint16_t>(base_address + registers_.x);
    bool is_page_crossed = (base_address & 0xFF00) != (address & 0xFF00);

    return {address, is_page_crossed};
}

CPU::AddressResult CPU::get_absolute_y_addr() {
    uint16_t base_address = get_absolute_address();
    uint16_t address = static_cast<uint16_t>(base_address + registers_.y);
    bool is_page_crossed = (base_address & 0xFF00) != (address & 0xFF00);

    return {address, is_page_crossed};
}

uint8_t CPU::rotate_left(uint8_t value) {
    uint8_t old_c = is_flag_active(FlagKind::C);

    set_flag(FlagKind::C, (value & 0x80) != 0);

    uint8_t result = static_cast<uint8_t>((value << 1) | old_c);

    set_zn_flags(result);
    return result;
}

uint8_t CPU::rotate_right(uint8_t value) {
    uint8_t old_c = is_flag_active(FlagKind::C);

    set_flag(FlagKind::C, (value & 0x01) != 0);

    uint8_t result = static_cast<uint8_t>((value >> 1) | (old_c << 7));

    set_zn_flags(result);
    return result;
}

uint8_t CPU::shift_left(uint8_t value) {
    set_flag(FlagKind::C, (value & 0x08) != 0);

    uint8_t result = static_cast<uint8_t>(value << 1);

    set_zn_flags(result);
    return result;
}

void CPU::execute() {
    uint8_t op = cpu_read(registers_.ip++);

    switch (op) {

        // =====================================================================
        // ADC - Add with Carry
        // =====================================================================

        // Immediate
        case 0x69: {
            add_with_carry(cpu_read(registers_.ip++));

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0x65: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            add_with_carry(cpu_read(zero_page_addr));

            add_cycles(3);
            break;
        }
        // Zero Page,X
        case 0x75: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            add_with_carry(cpu_read((zero_page_addr + registers_.x) % 256));

            add_cycles(4);
            break;
        }
        // Absolute
        case 0x6D: {
            uint16_t address = get_absolute_address();
            add_with_carry(cpu_read(address));

            add_cycles(4);
            break;
        }
        // Absolute,X
        case 0x7D: {
            AddressResult address_result = get_absolute_x_addr();
            add_with_carry(cpu_read(address_result.address));

            add_cycles(4);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }
        // Absolute,Y
        case 0x79: {
            AddressResult address_result = get_absolute_y_addr();
            add_with_carry(cpu_read(address_result.address));

            add_cycles(4);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }
        // Indexed Indirect (d,X)
        case 0x61: {
            AddressResult address_result = get_indexed_indirect_x_addr();
            add_with_carry(cpu_read(address_result.address));

            add_cycles(6);
            break;
        }
        // Indirect Indexed (d),Y
        case 0x71: {
            AddressResult address_result = get_indirect_indexed_y_addr();
            add_with_carry(cpu_read(address_result.address));

            add_cycles(5);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }

        // =====================================================================
        // AND - Bitwise AND
        // =====================================================================

        // Immediate
        case 0x29: {
            registers_.a &= cpu_read(registers_.ip++);
            set_zn_flags(registers_.a);

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0x25: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            registers_.a &= cpu_read(zero_page_addr);
            set_zn_flags(registers_.a);

            add_cycles(3);
            break;
        }
        // Zero Page,X
        case 0x35: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            registers_.a = registers_.a & cpu_read((zero_page_addr + registers_.x) % 256);
            set_zn_flags(registers_.a);

            add_cycles(4);
            break;
        }
        // Absolute
        case 0x2D: {
            uint16_t address = get_absolute_address();
            registers_.a = registers_.a & cpu_read(address);
            set_zn_flags(registers_.a);

            add_cycles(4);
            break;
        }
        // Absolute,X
        case 0x3D: {
            AddressResult address_result = get_absolute_x_addr();
            registers_.a = registers_.a & cpu_read(address_result.address);
            set_zn_flags(registers_.a);

            add_cycles(4);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }
        // Absolute,Y
        case 0x39: {
            AddressResult address_result = get_absolute_y_addr();
            registers_.a = registers_.a & cpu_read(address_result.address);
            set_zn_flags(registers_.a);

            add_cycles(4);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }
        // Indexed Indirect (d,X)
        case 0x21: {
            AddressResult address_result = get_indexed_indirect_x_addr();
            registers_.a = registers_.a & cpu_read(address_result.address);
            set_zn_flags(registers_.a);

            add_cycles(6);
            break;
        }
        // Indirect Indexed (d),Y
        case 0x31: {
            AddressResult address_result = get_indirect_indexed_y_addr();
            registers_.a = registers_.a & cpu_read(address_result.address);
            set_zn_flags(registers_.a);

            add_cycles(5);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }

        // =====================================================================
        // ASL - Arithmetic Shift Left
        // =====================================================================

        // Accumulator
        case 0x0A: {
            registers_.a = shift_left(registers_.a);

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0x06: {
            uint8_t addr = cpu_read(registers_.ip++);
            uint8_t result = shift_left(cpu_read(addr));

            cpu_write(addr, result);

            add_cycles(5);
            break;
        }
        // Zero Page,X
        case 0x16: {
            uint8_t addr = cpu_read(registers_.ip++);
            uint8_t result = shift_left(cpu_read((addr + registers_.x) % 256));

            cpu_write(addr, result);

            add_cycles(6);
            break;
        }
        // Absolute
        case 0x0E: {
            uint16_t addr = get_absolute_address();
            uint8_t result = shift_left(cpu_read(addr));

            cpu_write(addr, result);

            add_cycles(6);
            break;
        }
        // Absolute,X
        case 0x1E: {
            AddressResult addr_result = get_absolute_x_addr();
            uint8_t result = shift_left(cpu_read(addr_result.address + registers_.x));

            cpu_write(addr_result.address, result);

            add_cycles(7);
            break;
        }

        // =====================================================================
        // BCC - Branch if Carry Clear
        // =====================================================================

        // Relative
        case 0x90: {
            // Signed byte long offset
            int8_t offset = static_cast<int8_t>(cpu_read(registers_.ip++));

            if (!is_flag_active(FlagKind::C)) {
                uint16_t previous_ip = registers_.ip;
                registers_.ip += offset;
                add_cycles(1);
                if ((previous_ip & 0xFF00) != (registers_.ip & 0xFF00)) {
                    add_cycles(1);
                }
            }

            add_cycles(2);
            break;
        }

        // =====================================================================
        // BCS - Branch if Carry Set
        // =====================================================================

        // Relative
        case 0xB0: {
            int8_t offset = static_cast<int8_t>(cpu_read(registers_.ip++));

            if (is_flag_active(FlagKind::C)) {
                uint16_t previous_ip = registers_.ip;
                registers_.ip += offset;
                add_cycles(1);
                if ((previous_ip & 0xFF00) != (registers_.ip & 0xFF00)) {
                    add_cycles(1);
                }
            }

            add_cycles(2);
            break;
        }

        // =====================================================================
        // BEQ - Branch if Equal
        // =====================================================================

        // Relative (Branches if zero flag is set)
        case 0xF0: {
            int8_t offset = static_cast<int8_t>(cpu_read(registers_.ip++));

            if (is_flag_active(FlagKind::Z)) {
                uint16_t previous_ip = registers_.ip;
                registers_.ip += offset;
                add_cycles(1);
                if ((previous_ip & 0xFF00) != (registers_.ip & 0xFF00)) {
                    add_cycles(1);
                }
            }

            add_cycles(2);
            break;
        }

        // =====================================================================
        // BNE - Branch if Not Equal
        // =====================================================================

        // Relative
        case 0xD0: {
            int8_t offset = static_cast<int8_t>(cpu_read(registers_.ip++));

            if (!is_flag_active(FlagKind::Z)) {
                uint16_t previous_ip = registers_.ip;
                registers_.ip += offset;
                add_cycles(1);
                if ((previous_ip & 0xFF00) != (registers_.ip & 0xFF00)) {
                    add_cycles(1);
                }
            }

            add_cycles(2);
            break;
        }

        // =====================================================================
        // BMI - Branch if Minus
        // =====================================================================

        // Relative
        case 0x30: {
            int8_t offset = static_cast<int8_t>(cpu_read(registers_.ip++));

            if (is_flag_active(FlagKind::N)) {
                uint16_t previous_ip = registers_.ip;
                registers_.ip += offset;
                add_cycles(1);
                if ((previous_ip & 0xFF00) != (registers_.ip & 0xFF00)) {
                    add_cycles(1);
                }
            }

            add_cycles(2);
            break;
        }

        // =====================================================================
        // BPL - Branch if Plus
        // =====================================================================

        // Relative
        case 0x10: {
            int8_t offset = static_cast<int8_t>(cpu_read(registers_.ip++));

            if (!is_flag_active(FlagKind::N)) {
                uint16_t previous_ip = registers_.ip;
                registers_.ip += offset;
                add_cycles(1);
                if ((previous_ip & 0xFF00) != (registers_.ip & 0xFF00)) {
                    add_cycles(1);
                }
            }

            add_cycles(2);
            break;
        }

        // =====================================================================
        // BRK - Break (software IRQ)
        // =====================================================================

        // Implied & Immediate
        case 0x00: {
            uint16_t return_addr = registers_.ip + 2;
            uint8_t high = static_cast<uint8_t>(return_addr >> 8);
            uint8_t low = static_cast<uint8_t>(return_addr & 0x0F);
            uint8_t sr = (registers_.sr & 0xCF) | 0x20;

            stack_push(high);
            stack_push(low);
            stack_push(sr);

            // Jump to interrupt handler code
            registers_.sp = static_cast<uint8_t>(0xFFFE);
            break;
        }

        // =====================================================================
        // BVC - Branch if Overflow Clear
        // =====================================================================

        // Relative
        case 0x50: {
            int8_t offset = static_cast<int8_t>(cpu_read(registers_.ip++));

            if (!is_flag_active(FlagKind::V)) {
                uint16_t previous_ip = registers_.ip;
                registers_.ip += offset;
                add_cycles(1);
                if ((previous_ip & 0xFF00) != (registers_.ip & 0xFF00)) {
                    add_cycles(1);
                }
            }

            add_cycles(2);
            break;
        }

        // =====================================================================
        // BVS - Branch if Overflow Set
        // =====================================================================

        // Relative
        case 0x70: {
            int8_t offset = static_cast<int8_t>(cpu_read(registers_.ip++));

            if (is_flag_active(FlagKind::V)) {
                uint16_t previous_ip = registers_.ip;
                registers_.ip += offset;
                add_cycles(1);
                if ((previous_ip & 0xFF00) != (registers_.ip & 0xFF00)) {
                    add_cycles(1);
                }
            }

            add_cycles(2);
            break;
        }
        // =====================================================================
        // BIT - Bit Test
        // =====================================================================

        // Zero Page
        case 0x24: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            uint8_t operand = cpu_read(zero_page_addr);
            uint8_t result = registers_.a & operand;
            set_flag(FlagKind::Z, result == 0);
            set_flag(FlagKind::V, (operand & 0x40));
            set_flag(FlagKind::N, (operand & 0x80));

            add_cycles(3);
            break;
        }
        // Absolute
        case 0x2C: {
            uint16_t address = get_absolute_address();
            uint8_t operand = cpu_read(address);
            uint8_t result = registers_.a & operand;
            set_flag(FlagKind::Z, result == 0);
            set_flag(FlagKind::V, (operand & 0x40));
            set_flag(FlagKind::N, (operand & 0x80));

            add_cycles(4);
            break;
        }

        // =====================================================================
        // CLC - Clear Carry Flag
        // =====================================================================

        // Implied
        case 0x18: {
            set_flag(FlagKind::C, false);

            add_cycles(2);
            break;
        }

        // =====================================================================
        // CLD - Clear Decimal Flag
        // =====================================================================

        // Implied
        case 0xD8: {
            set_flag(FlagKind::D, false);

            add_cycles(2);
            break;
        }

        // =====================================================================
        // CLI - Clear Interrupt Disable Flag
        // =====================================================================

        // Implied
        case 0x58: {
            set_flag(FlagKind::I, false);

            add_cycles(2);
            break;
        }

        // =====================================================================
        // CLV - Clear Overflow Flag
        // =====================================================================

        // Implied
        case 0xB8: {
            set_flag(FlagKind::V, false);

            add_cycles(2);
            break;
        }

        // =====================================================================
        // CMP - Compare Accumulator
        // =====================================================================

        // Immediate
        case 0xC9: {
            uint8_t operand = cpu_read(registers_.ip++);
            compare(registers_.a, operand);

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0xC5: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            uint8_t operand = cpu_read(zero_page_addr);
            compare(registers_.a, operand);

            add_cycles(3);
            break;
        }
        // Zero Page,X
        case 0xD5: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            uint8_t operand = cpu_read((zero_page_addr + registers_.x) % 256);
            compare(registers_.a, operand);

            add_cycles(4);
            break;
        }
        // Absolute
        case 0xCD: {
            uint16_t address = get_absolute_address();
            uint8_t operand = cpu_read(address);
            compare(registers_.a, operand);

            add_cycles(4);
            break;
        }
        // Absolute,X
        case 0xDD: {
            AddressResult address_result = get_absolute_x_addr();
            uint8_t operand = cpu_read(address_result.address);
            compare(registers_.a, operand);

            add_cycles(4);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }
        // Absolute,Y
        case 0xD9: {
            AddressResult address_result = get_absolute_y_addr();
            uint8_t operand = cpu_read(address_result.address);
            compare(registers_.a, operand);

            add_cycles(4);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }
        // Indexed Indirect (d,X)
        case 0xC1: {
            AddressResult address_result = get_indexed_indirect_x_addr();
            uint8_t operand = cpu_read(address_result.address);
            compare(registers_.a, operand);

            add_cycles(6);
            break;
        }
        // Indirect Indexed (d),Y
        case 0xD1: {
            AddressResult address_result = get_indirect_indexed_y_addr();
            uint8_t operand = cpu_read(address_result.address);
            compare(registers_.a, operand);

            add_cycles(5);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }

        // =====================================================================
        // CPX - Compare X Register
        // =====================================================================

        // Immediate
        case 0xE0: {
            uint8_t operand = cpu_read(registers_.ip++);
            compare(registers_.x, operand);

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0xE4: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            uint8_t operand = cpu_read(zero_page_addr);
            compare(registers_.x, operand);

            add_cycles(3);
            break;
        }
        // Absolute
        case 0xEC: {
            uint16_t address = get_absolute_address();
            uint8_t operand = cpu_read(address);
            compare(registers_.x, operand);

            add_cycles(4);
            break;
        }

        // =====================================================================
        // CPY - Compare Y Register
        // =====================================================================

        // Immediate
        case 0xC0: {
            uint8_t operand = cpu_read(registers_.ip++);
            compare(registers_.y, operand);

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0xC4: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            uint8_t operand = cpu_read(zero_page_addr);
            compare(registers_.y, operand);

            add_cycles(3);
            break;
        }
        // Absolute
        case 0xCC: {
            uint16_t address = get_absolute_address();
            uint8_t operand = cpu_read(address);
            compare(registers_.y, operand);

            add_cycles(4);
            break;
        }

        // =====================================================================
        // DEC - Decrement Memory
        // =====================================================================

        // Zero Page
        case 0xC6: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            uint8_t operand = cpu_read(zero_page_addr);
            uint8_t result = operand - 1;
            memory_.at(zero_page_addr) = result;
            set_flag(FlagKind::Z, result == 0);
            set_flag(FlagKind::N, (result & 0x80));

            add_cycles(5);
            break;
        }
        // Zero Page,X
        case 0xD6: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            uint8_t operand = cpu_read((zero_page_addr + registers_.x) % 256);
            uint8_t result = operand - 1;
            memory_.at((zero_page_addr + registers_.x) % 256) = result;
            set_flag(FlagKind::Z, result == 0);
            set_flag(FlagKind::N, (result & 0x80));

            add_cycles(6);
            break;
        }
        // Absolute
        case 0xCE: {
            uint16_t address = get_absolute_address();
            uint8_t operand = cpu_read(address);
            uint8_t result = operand - 1;
            memory_.at(address) = result;
            set_flag(FlagKind::Z, result == 0);
            set_flag(FlagKind::N, (result & 0x80));

            add_cycles(6);
            break;
        }
        // Absolute,X
        case 0xDE: {
            AddressResult address_result = get_absolute_x_addr();
            uint8_t operand = cpu_read(address_result.address);
            uint8_t result = operand - 1;
            memory_.at(address_result.address) = result;
            set_flag(FlagKind::Z, result == 0);
            set_flag(FlagKind::N, (result & 0x80));

            add_cycles(7);
            break;
        }

        // =====================================================================
        // DEX - Decrement X Register
        // =====================================================================

        // Implied
        case 0xCA: {
            uint8_t result = registers_.x - 1;
            registers_.x = result;
            set_flag(FlagKind::Z, result == 0);
            set_flag(FlagKind::N, (result & 0x80));

            add_cycles(2);
            break;
        }

        // =====================================================================
        // DEY - Decrement Y Register
        // =====================================================================

        // Implied
        case 0x88: {
            uint8_t result = registers_.y - 1;
            registers_.y = result;
            set_flag(FlagKind::Z, result == 0);
            set_flag(FlagKind::N, (result & 0x80));

            add_cycles(2);
            break;
        }

        // =====================================================================
        // EOR - Bitwise Exclusive OR
        // =====================================================================

        // Immediate
        case 0x49: {
            uint8_t operand = cpu_read(registers_.ip++);
            uint8_t result = registers_.a ^ operand;
            registers_.a = result;
            set_zn_flags(result);

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0x45: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            uint8_t operand = cpu_read(zero_page_addr);
            uint8_t result = registers_.a ^ operand;
            registers_.a = result;
            set_zn_flags(result);

            add_cycles(3);
            break;
        }
        // Zero Page,X
        case 0x55: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            uint8_t operand = cpu_read((zero_page_addr + registers_.x) % 256);
            uint8_t result = registers_.a ^ operand;
            registers_.a = result;
            set_zn_flags(result);

            add_cycles(4);
            break;
        }
        // Absolute
        case 0x4D: {
            uint16_t address = get_absolute_address();
            uint8_t operand = cpu_read(address);
            uint8_t result = registers_.a ^ operand;
            registers_.a = result;
            set_zn_flags(result);

            add_cycles(4);
            break;
        }
        // Absolute,X
        case 0x5D: {
            AddressResult address_result = get_absolute_x_addr();
            uint8_t operand = cpu_read(address_result.address);
            uint8_t result = registers_.a ^ operand;
            registers_.a = result;
            set_zn_flags(result);

            add_cycles(4);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }
        // Absolute,Y
        case 0x59: {
            AddressResult address_result = get_absolute_y_addr();
            uint8_t operand = cpu_read(address_result.address);
            uint8_t result = registers_.a ^ operand;
            registers_.a = result;
            set_zn_flags(result);

            add_cycles(4);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }
        // Indexed Indirect (d,X)
        case 0x41: {
            AddressResult address_result = get_indexed_indirect_x_addr();
            uint8_t operand = cpu_read(address_result.address);
            uint8_t result = registers_.a ^ operand;
            registers_.a = result;
            set_zn_flags(result);

            add_cycles(6);
            break;
        }
        // Indirect Indexed (d),Y
        case 0x51: {
            AddressResult address_result = get_indirect_indexed_y_addr();
            uint8_t operand = cpu_read(address_result.address);
            uint8_t result = registers_.a ^ operand;
            registers_.a = result;
            set_zn_flags(result);

            add_cycles(5);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }

        // =====================================================================
        // INC - Increment Memory
        // =====================================================================

        // Zero Page
        case 0xE6: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            uint8_t result = static_cast<uint8_t>(cpu_read(zero_page_addr) + 1);
            cpu_write(zero_page_addr, result);
            set_zn_flags(result);

            add_cycles(5);
            break;
        }
        // Zero Page,X
        case 0xF6: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            uint8_t address = (zero_page_addr + registers_.x) % 256;
            uint8_t result = static_cast<uint8_t>(cpu_read(address) + 1);
            cpu_write(address, result);
            set_zn_flags(result);

            add_cycles(6);
            break;
        }
        // Absolute
        case 0xEE: {
            uint16_t address = get_absolute_address();
            uint8_t result = static_cast<uint8_t>(cpu_read(address) + 1);
            cpu_write(address, result);
            set_zn_flags(result);

            add_cycles(6);
            break;
        }
        // Absolute,X
        case 0xFE: {
            AddressResult address_result = get_absolute_x_addr();
            uint8_t result = static_cast<uint8_t>(cpu_read(address_result.address) + 1);
            cpu_write(address_result.address, result);
            set_zn_flags(result);

            add_cycles(7);
            break;
        }

        // =====================================================================
        // INX - Increment X Register
        // =====================================================================

        // Implied
        case 0xE8: {
            registers_.x++;
            set_zn_flags(registers_.x);

            add_cycles(2);
            break;
        }

        // =====================================================================
        // INY - Increment Y Register
        // =====================================================================

        // Implied
        case 0xC8: {
            registers_.y++;
            set_zn_flags(registers_.y);

            add_cycles(2);
            break;
        }

        // =====================================================================
        // JMP - Jump
        // =====================================================================

        // Absolute
        case 0x4C: {
            registers_.ip = get_absolute_address();

            add_cycles(3);
            break;
        }
        // Indirect (JMP's own special addressing mode)
        case 0x6C: {
            uint16_t pointer_addr = get_absolute_address();
            registers_.ip = pointer_addr;
            uint8_t low = cpu_read(registers_.ip++);
            uint8_t high = cpu_read(registers_.ip);

            // Replicate the wrap-around hardware bug:
            // When the low byte is 0xFF, the high byte doesn't increment
            if (low == 0xFF) {
                registers_.ip = static_cast<uint16_t>(high << 8 | low);
            } else {
                registers_.ip = static_cast<uint16_t>(high << 8 | low + 1);
            }

            add_cycles(5);
            break;
        }

        // =====================================================================
        // JSR - Jump to Subroutine
        // =====================================================================

        // Absolute
        case 0x20: {
            uint16_t return_addr = registers_.ip + 2;
            uint8_t high = static_cast<uint8_t>(return_addr >> 8);
            uint8_t low = static_cast<uint8_t>(return_addr & 0x0F);

            cpu_write(0x0100 + registers_.sp, high);
            registers_.sp--;
            cpu_write(0x0100 + registers_.sp, low);
            registers_.sp--;

            registers_.ip = get_absolute_address();

            add_cycles(6);
            break;
        }

        // =====================================================================
        // LDA - Load Accumulator
        // =====================================================================

        // Immediate
        case 0xA9: {
            uint8_t operand = cpu_read(registers_.ip++);
            registers_.a = operand;
            set_zn_flags(registers_.a);

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0xA5: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            registers_.a = cpu_read(zero_page_addr);
            set_zn_flags(registers_.a);

            add_cycles(3);
            break;
        }
        // Zero Page,X
        case 0xB5: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            registers_.a = cpu_read((zero_page_addr + registers_.x) % 256);
            set_zn_flags(registers_.a);

            add_cycles(4);
            break;
        }
        // Absolute
        case 0xAD: {
            uint16_t address = get_absolute_address();
            registers_.a = cpu_read(address);
            set_zn_flags(registers_.a);

            add_cycles(4);
            break;
        }
        // Absolute,X
        case 0xBD: {
            AddressResult address_result = get_absolute_x_addr();
            registers_.a = cpu_read(address_result.address);
            set_zn_flags(registers_.a);

            add_cycles(4);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }
        // Absolute,Y
        case 0xB9: {
            AddressResult address_result = get_absolute_y_addr();
            registers_.a = cpu_read(address_result.address);
            set_zn_flags(registers_.a);

            add_cycles(4);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }
        // Indexed Indirect (d,X)
        case 0xA1: {
            AddressResult address_result = get_indexed_indirect_x_addr();
            registers_.a = cpu_read(address_result.address);
            set_zn_flags(registers_.a);

            add_cycles(6);
            break;
        }
        // Indirect Indexed (d), Y
        case 0xB1: {
            AddressResult address_result = get_indirect_indexed_y_addr();
            registers_.a = cpu_read(address_result.address);
            set_zn_flags(registers_.a);

            add_cycles(5);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }

        // =====================================================================
        // LDX - Load X Register
        // =====================================================================

        // Immediate
        case 0xA2: {
            uint8_t operand = cpu_read(registers_.ip++);
            registers_.x = operand;
            set_zn_flags(registers_.x);

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0xA6: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            registers_.x = cpu_read(zero_page_addr);
            set_zn_flags(registers_.x);

            add_cycles(3);
            break;
        }
        // Zero Page,Y
        case 0xB6: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            registers_.x = cpu_read((zero_page_addr + registers_.y) % 256);
            set_zn_flags(registers_.x);

            add_cycles(4);
            break;
        }
        // Absolute
        case 0xAE: {
            uint16_t address = get_absolute_address();
            registers_.x = cpu_read(address);
            set_zn_flags(registers_.x);

            add_cycles(4);
            break;
        }
        // Absolute,Y
        case 0xBE: {
            AddressResult address_result = get_absolute_y_addr();
            registers_.x = cpu_read(address_result.address);
            set_zn_flags(registers_.x);

            add_cycles(4);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }

        // =====================================================================
        // LDY - Load Y Register
        // =====================================================================

        // Immediate
        case 0xA0: {
            uint8_t operand = cpu_read(registers_.ip++);
            registers_.y = operand;
            set_zn_flags(registers_.y);

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0xA4: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            registers_.y = cpu_read(zero_page_addr);
            set_zn_flags(registers_.y);

            add_cycles(3);
            break;
        }
        // Zero Page,X
        case 0xB4: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            registers_.y = cpu_read((zero_page_addr + registers_.x) % 256);
            set_zn_flags(registers_.y);

            add_cycles(4);
            break;
        }
        // Absolute
        case 0xAC: {
            uint16_t address = get_absolute_address();
            registers_.y = cpu_read(address);
            set_zn_flags(registers_.y);

            add_cycles(4);
            break;
        }
        // Absolute,X
        case 0xBC: {
            AddressResult address_result = get_absolute_x_addr();
            registers_.y = cpu_read(address_result.address);
            set_zn_flags(registers_.y);

            add_cycles(4);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }

        // =====================================================================
        // LSR - Logical Shift Right
        // =====================================================================

        // Accumulator
        case 0x4A: {
            uint8_t original_value = registers_.a;
            set_flag(FlagKind::C, (original_value & 0x01) != 0);
            uint8_t result = original_value >> 1;
            registers_.a = result;
            set_zn_flags(result);

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0x46: {
            uint8_t addr = cpu_read(registers_.ip++);
            uint8_t original_value = cpu_read(addr);
            set_flag(FlagKind::C, (original_value & 0x01) != 0);
            uint8_t result = original_value >> 1;
            memory_.at(addr) = result;
            set_zn_flags(result);

            add_cycles(5);
            break;
        }
        // Zero Page,X
        case 0x56: {
            uint8_t addr = cpu_read(registers_.ip++);
            uint8_t original_value = cpu_read((addr + registers_.x) % 256);
            set_flag(FlagKind::C, (original_value & 0x01) != 0);
            uint8_t result = original_value >> 1;
            memory_.at(addr) = result;
            set_zn_flags(result);

            add_cycles(6);
            break;
        }
        // Absolute
        case 0x4E: {
            uint16_t address = get_absolute_address();
            uint8_t original_value = cpu_read(address);
            set_flag(FlagKind::C, (original_value & 0x01) != 0);
            uint8_t result = original_value >> 1;
            memory_.at(address) = result;
            set_zn_flags(result);

            add_cycles(6);
            break;
        }
        // Absolute,X
        case 0x5E: {
            AddressResult address_result = get_absolute_x_addr();
            uint8_t original_value = cpu_read(address_result.address);
            set_flag(FlagKind::C, (original_value & 0x01) != 0);
            uint8_t result = original_value >> 1;
            memory_.at(address_result.address) = result;
            set_zn_flags(result);

            add_cycles(7);
            break;
        }

        // =====================================================================
        // NOP - No Operation
        // =====================================================================

        // Implied
        case 0xEA: {
            add_cycles(2);
            break;
        }

        // =====================================================================
        // ORA - Bitwise OR
        // =====================================================================

        // Immediate
        case 0x09: {
            uint8_t operand = cpu_read(registers_.ip++);
            registers_.a |= operand;
            set_zn_flags(registers_.a);

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0x05: {
            uint8_t addr = cpu_read(registers_.ip++);
            uint8_t operand = cpu_read(addr);
            registers_.a |= operand;
            set_zn_flags(registers_.a);

            add_cycles(3);
            break;
        }
        // Zero Page,X
        case 0x15: {
            uint8_t addr = cpu_read(registers_.ip++);
            uint8_t operand = cpu_read((addr + registers_.x) % 256);
            registers_.a |= operand;
            set_zn_flags(registers_.a);

            add_cycles(4);
            break;
        }
        // Absolute
        case 0x0D: {
            uint16_t address = get_absolute_address();
            uint8_t operand = cpu_read(address);
            registers_.a |= operand;
            set_zn_flags(registers_.a);

            add_cycles(4);
            break;
        }
        // Absolute,X
        case 0x1D: {
            AddressResult address_result = get_absolute_x_addr();
            uint8_t operand = cpu_read(address_result.address);
            registers_.a |= operand;
            set_zn_flags(registers_.a);

            add_cycles(4);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }
        // Absolute,Y
        case 0x19: {
            AddressResult address_result = get_absolute_y_addr();
            uint8_t operand = cpu_read(address_result.address);
            registers_.a |= operand;
            set_zn_flags(registers_.a);

            add_cycles(4);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }
        // Indexed Indirect (d,X)
        case 0x01: {
            AddressResult address_result = get_indexed_indirect_x_addr();
            uint8_t operand = cpu_read(address_result.address);
            registers_.a |= operand;
            set_zn_flags(registers_.a);

            add_cycles(6);
            break;
        }
        // Indirect Indexed (d),Y
        case 0x11: {
            AddressResult address_result = get_indirect_indexed_y_addr();
            uint8_t operand = cpu_read(address_result.address);
            registers_.a |= operand;
            set_zn_flags(registers_.a);

            add_cycles(5);
            if (address_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }

        // =====================================================================
        // PHA - Push A
        // =====================================================================

        // Implied
        case 0x48: {
            stack_push(registers_.a);

            add_cycles(3);
            break;
        }

        // =====================================================================
        // PHP - Push Processor Status
        // =====================================================================

        // Implied
        case 0x08: {
            cpu_write(0x0100 + registers_.sp, registers_.sr | 0x10);
            registers_.sp--;

            add_cycles(3);
            break;
        }

        // =====================================================================
        // PLA - Pull A
        // =====================================================================

        // Implied
        case 0x68: {
            registers_.a = stack_pop();
            set_zn_flags(registers_.a);

            add_cycles(4);
            break;
        }

        // =====================================================================
        // PLP - Pull Processor Status
        // =====================================================================

        // Implied
        case 0x28: {
            registers_.sp++;
            uint8_t pulled_status = cpu_read(0x0100 + registers_.sp);
            registers_.sr = (registers_.sr & 0x20) | (pulled_status & 0xCF);

            add_cycles(4);
            break;
        }

        // =====================================================================
        // ROL - Rotate Left
        // =====================================================================

        // Accumulator
        case 0x2A: {
            registers_.a = rotate_left(registers_.a);

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0x26: {
            uint8_t addr = cpu_read(registers_.ip++);
            uint8_t result = rotate_left(cpu_read(addr));

            cpu_write(addr, result);

            add_cycles(5);
            break;
        }
        // Zero Page,X
        case 0x36: {
            uint8_t addr = cpu_read(registers_.ip++);
            uint8_t result = rotate_left(cpu_read((addr + registers_.x) % 256));

            cpu_write(addr, result);

            add_cycles(6);
            break;
        }
        // Absolute
        case 0x2E: {
            uint16_t addr = get_absolute_address();
            uint8_t result = rotate_left(cpu_read(addr));

            cpu_write(addr, result);

            add_cycles(6);
            break;
        }
        // Absolute,X
        case 0x3E: {
            AddressResult address_result = get_absolute_x_addr();
            uint8_t result = rotate_left(cpu_read(address_result.address));

            cpu_write(address_result.address, result);

            add_cycles(7);
            break;
        }

        // =====================================================================
        // ROR - Rotate Right
        // =====================================================================

        // Accumulator
        case 0x6A: {
            registers_.a = rotate_right(registers_.a);

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0x66: {
            uint8_t addr = cpu_read(registers_.ip++);
            uint8_t result = rotate_right(cpu_read(addr));

            cpu_write(addr, result);

            add_cycles(5);
            break;
        }
        // Zero Page,X
        case 0x76: {
            uint8_t addr = cpu_read(registers_.ip++);
            uint8_t result = rotate_right(cpu_read((addr + registers_.x) % 256));

            cpu_write(addr, result);

            add_cycles(6);
            break;
        }
        // Absolute
        case 0x6E: {
            uint16_t addr = get_absolute_address();
            uint8_t result = rotate_right(cpu_read(addr));

            cpu_write(addr, result);

            add_cycles(6);
            break;
        }
        // Absolute,X
        case 0x7E: {
            AddressResult address_result = get_absolute_x_addr();
            uint8_t result = rotate_right(cpu_read(address_result.address));

            cpu_write(address_result.address, result);

            add_cycles(7);
            break;
        }

        // =====================================================================
        // RTI - Return From Interrupt
        // =====================================================================

        // Implied
        case 0x40: {
            uint8_t restored_status = (stack_pop() & 0xCF) | 0x20;
            uint8_t low = stack_pop();
            uint8_t high = stack_pop();

            registers_.ip = (high << 8) | low;
            registers_.sr = restored_status;

            add_cycles(6);
            break;
        }

        // =====================================================================
        // RTS - Return From Subroutine
        // =====================================================================

        // Implied
        case 0x60: {
            uint8_t low = stack_pop();
            uint8_t high = stack_pop();

            registers_.ip = (high << 8) | low;
            registers_.ip++;

            add_cycles(6);
            break;
        }

        // =====================================================================
        // SBC - Subtract with Carry
        // =====================================================================

        // Immediate
        case 0xE9: {
            uint8_t operand = ~cpu_read(registers_.ip++);
            add_with_carry(operand);

            add_cycles(2);
            break;
        }
        // Zero Page
        case 0xE5: {
            uint8_t addr = cpu_read(registers_.ip++);
            add_with_carry(~cpu_read(addr));

            add_cycles(3);
            break;
        }
        // Zero Page,X
        case 0xF5: {
            uint8_t addr = cpu_read(registers_.ip++);
            add_with_carry(~cpu_read((addr + registers_.x) % 256));

            add_cycles(4);
            break;
        }
        // Absolute
        case 0xED: {
            uint16_t addr = get_absolute_address();
            add_with_carry(~cpu_read(addr));

            add_cycles(4);
            break;
        }
        // Absolute,X
        case 0xFD: {
            AddressResult addr_result = get_absolute_x_addr();
            add_with_carry(~cpu_read(addr_result.address));

            add_cycles(4);
            if (addr_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }
        // Absolute,Y
        case 0xF9: {
            AddressResult addr_result = get_absolute_y_addr();
            add_with_carry(~cpu_read(addr_result.address));

            add_cycles(4);
            if (addr_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }
        // Indexed Indirect (d,X)
        case 0xE1: {
            AddressResult addr_result = get_indexed_indirect_x_addr();
            add_with_carry(~cpu_read(addr_result.address));

            add_cycles(6);
            break;
        }
        // Indirect Indexed (d),Y
        case 0xF1: {
            AddressResult addr_result = get_indirect_indexed_y_addr();
            add_with_carry(~cpu_read(addr_result.address));

            add_cycles(5);
            if (addr_result.is_page_crossed) {
                add_cycles(1);
            }
            break;
        }

        // =====================================================================
        // SEC - Set Carry Flag
        // =====================================================================

        // Implied
        case 0x38: {
            set_flag(FlagKind::C, true);

            add_cycles(2);
            break;
        }

        // =====================================================================
        // SED - Set Decimal Flag
        // =====================================================================

        // Implied
        case 0xF8: {
            set_flag(FlagKind::D, true);

            add_cycles(2);
            break;
        }

        // =====================================================================
        // SEI - Set Interrupt Disable Flag
        // =====================================================================

        // Implied
        case 0x78: {
            set_flag(FlagKind::I, true);

            add_cycles(2);
            break;
        }

        // =====================================================================
        // STA - Store Accumulator
        // =====================================================================

        // Zero Page
        case 0x85: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            cpu_write(zero_page_addr, registers_.a);

            add_cycles(3);
            break;
        }
        // Zero Page,X
        case 0x95: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            uint8_t address = (zero_page_addr + registers_.x) % 256;
            cpu_write(address, registers_.a);

            add_cycles(4);
            break;
        }
        // Absolute
        case 0x8D: {
            uint16_t address = get_absolute_address();
            cpu_write(address, registers_.a);

            add_cycles(4);
            break;
        }
        // Absolute,X
        case 0x9D: {
            AddressResult address_result = get_absolute_x_addr();
            cpu_write(address_result.address, registers_.a);

            add_cycles(5);
            break;
        }
        // Absolute,Y
        case 0x99: {
            AddressResult address_result = get_absolute_y_addr();
            cpu_write(address_result.address, registers_.a);

            add_cycles(5);
            break;
        }
        // Indexed Indirect (d,X)
        case 0x81: {
            AddressResult address_result = get_indexed_indirect_x_addr();
            cpu_write(address_result.address, registers_.a);

            add_cycles(6);
            break;
        }
        // Indirect Indexed (d),Y
        case 0x91: {
            AddressResult address_result = get_indirect_indexed_y_addr();
            cpu_write(address_result.address, registers_.a);

            add_cycles(6);
            break;
        }

        // =====================================================================
        // STX - Store X Register
        // =====================================================================

        // Zero Page
        case 0x86: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            cpu_write(zero_page_addr, registers_.x);

            add_cycles(3);
            break;
        }
        // Zero Page,Y
        case 0x96: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            uint8_t address = (zero_page_addr + registers_.y) % 256;
            cpu_write(address, registers_.x);

            add_cycles(4);
            break;
        }
        // Absolute
        case 0x8E: {
            uint16_t address = get_absolute_address();
            cpu_write(address, registers_.x);

            add_cycles(4);
            break;
        }

        // =====================================================================
        // STY - Store Y Register
        // =====================================================================

        // Zero Page
        case 0x84: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            cpu_write(zero_page_addr, registers_.y);

            add_cycles(3);
            break;
        }
        // Zero Page,X
        case 0x94: {
            uint8_t zero_page_addr = cpu_read(registers_.ip++);
            uint8_t address = (zero_page_addr + registers_.x) % 256;
            cpu_write(address, registers_.y);

            add_cycles(4);
            break;
        }
        // Absolute
        case 0x8C: {
            uint16_t address = get_absolute_address();
            cpu_write(address, registers_.y);

            add_cycles(4);
            break;
        }

        // =====================================================================
        // TAX - Transfer Accumulator to X
        // =====================================================================

        // Implied
        case 0xAA: {
            registers_.x = registers_.a;
            set_zn_flags(registers_.x);

            add_cycles(2);
            break;
        }

        // =====================================================================
        // TAY - Transfer Accumulator to Y
        // =====================================================================

        // Implied
        case 0xA8: {
            registers_.y = registers_.a;
            set_zn_flags(registers_.y);

            add_cycles(2);
            break;
        }

        // =====================================================================
        // TSX - Transfer Stack Pointer to X
        // =====================================================================

        // Implied
        case 0xBA: {
            registers_.x = registers_.sp;
            set_zn_flags(registers_.x);

            add_cycles(2);
            break;
        }

        // =====================================================================
        // TXA - Transfer X to Accumulator
        // =====================================================================

        // Implied
        case 0x8A: {
            registers_.a = registers_.x;
            set_zn_flags(registers_.a);

            add_cycles(2);
            break;
        }

        // =====================================================================
        // TXS - Transfer X to Stack Pointer
        // =====================================================================

        // Implied
        case 0x9A: {
            registers_.sp = registers_.x;

            add_cycles(2);
            break;
        }

        // =====================================================================
        // TYA - Transfer Y to Accumulator
        // =====================================================================

        // Implied
        case 0x98: {
            registers_.a = registers_.y;
            set_zn_flags(registers_.a);

            add_cycles(2);
            break;
        }
    }
}
