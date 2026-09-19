#pragma once

#include <array>
#include <cstdint>

class Cartridge;
class Bus;

class CPU {
    enum FlagKind {
        // Carry Bit 0 - If Result > 0xFF, unsigned overflow occurred
        // If Result < 0x00, unsigned underflow occurred
        C,
        // Zero Bit 1 - Checks if result or data loaded == 0x00
        Z,
        // Interrupt Bit 2 - When on, hardware interrupts are blocked and ignored by
        // the CPU
        I,
        // Decimal Bit 3 - Tells us whether decimal mode is active or not
        D,
        // Break Bit 4 - Tells us whether an interrupt was caused by a software BRK vs
        // hardware
        B,
        // Unused Bit 5 - Always constant and ignored in hardware behaviour
        U,
        // Overflow Bit 6 - Set during arithmetic when an underflow or overflow breaks
        // two complement bounds.
        V,
        // Negative Bit 7 - Set if the highest bit of the result is set, representing
        // a negative number in signed binary
        N
    };

    public:
    CPU() = default;

    struct Registers {
        // 16-bit program counter or instruction pointer
        uint16_t ip;
        // 8-bit stack pointer register, selects a location in the stack page
        uint8_t sp;
        // 8-bit status register [NV1B DIZC] (bit 4 & 5 not used)
        uint8_t sr;
        // 8-bit wide accumulator
        uint8_t a;
        // 8-bit index X
        uint8_t x;
        // 8-bit index Y
        uint8_t y;
    };

    struct AddressResult {
        uint16_t address{};
        bool is_page_crossed{};
    };

    void execute();

    // Status Register Helpers
    uint8_t extract(uint8_t data, uint8_t mask, uint8_t shift = 0) const;
    void set_flag(FlagKind kind, bool active);
    bool is_flag_active(FlagKind kind);

    // Instruction Helpers
    void set_zn_flags(uint8_t reg);
    void compare(uint8_t reg, uint8_t operand);
    uint8_t rotate_left(uint8_t value);
    uint8_t rotate_right(uint8_t value);
    uint8_t shift_left(uint8_t value);
    void add_with_carry(uint16_t operand);

    // Addressing Helpers
    AddressResult get_indexed_indirect_x_addr();
    AddressResult get_indirect_indexed_y_addr();
    uint16_t get_absolute_address();
    AddressResult get_absolute_x_addr();
    AddressResult get_absolute_y_addr();

    // CPU Memory Access
    uint8_t cpu_read(uint16_t address) const;
    void cpu_write(uint16_t address, uint8_t value);

    // Stack Operations
    uint8_t stack_pop();
    void stack_push(uint8_t value);

    // Cycle Tracking
    uint64_t get_cycles() const { return cycles_; }
    void set_cycles(uint64_t cycles) { cycles_ = cycles; }
    void add_cycles(uint64_t cycles) { cycles_ += cycles; }

    // Debugging and Test State
    const Registers& get_registers() const { return registers_; }
    void set_registers(const Registers& registers) { registers_ = registers; }

    // Component Connections
    void set_cartridge(Cartridge* cartdridge) { cartridge_ = cartdridge; }

    private:
    // Access to PRG-ROM
    Cartridge* cartridge_ = nullptr;
    // 2KB onboard memory from [0x0000, 0x07FF]
    std::array<uint8_t, 2048> memory_{};
    Registers registers_{};
    Bus* bus_ = nullptr;

    uint64_t cycles_{};
};
