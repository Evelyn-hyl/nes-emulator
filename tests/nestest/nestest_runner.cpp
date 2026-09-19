#include "cartridge.hpp"
#include "cpu.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace {
    struct ExpectedState {
        uint16_t ip{};
        uint8_t a{};
        uint8_t x{};
        uint8_t y{};
        uint8_t sr{};
        uint8_t sp{};
        uint64_t cycles{};
        std::size_t source_line{};
        bool is_unofficial{};
        std::string raw_line;
    };

    uint64_t parse_number(const std::string& value, int base) { return std::stoull(value, nullptr, base); }

    bool parse_reference_line(const std::string& line, std::size_t source_line, ExpectedState& state) {
        static const std::regex state_pattern(
            R"(^([0-9A-Fa-f]{4})\s+.*\bA:([0-9A-Fa-f]{2})\s+X:([0-9A-Fa-f]{2})\s+Y:([0-9A-Fa-f]{2})\s+P:([0-9A-Fa-f]{2})\s+SP:([0-9A-Fa-f]{2}).*\bCYC:\s*([0-9]+))"
        );

        std::smatch matches;
        if (!std::regex_search(line, matches, state_pattern)) {
            return false;
        }

        state.ip = static_cast<uint16_t>(parse_number(matches[1].str(), 16));
        state.a = static_cast<uint8_t>(parse_number(matches[2].str(), 16));
        state.x = static_cast<uint8_t>(parse_number(matches[3].str(), 16));
        state.y = static_cast<uint8_t>(parse_number(matches[4].str(), 16));
        state.sr = static_cast<uint8_t>(parse_number(matches[5].str(), 16));
        state.sp = static_cast<uint8_t>(parse_number(matches[6].str(), 16));
        state.cycles = parse_number(matches[7].str(), 10);
        state.source_line = source_line;
        state.is_unofficial = line.find('*') != std::string::npos;
        state.raw_line = line;
        return true;
    }

    std::string format_state(uint16_t ip, uint8_t a, uint8_t x, uint8_t y, uint8_t sr, uint8_t sp, uint64_t cycles) {
        std::ostringstream output;
        output << std::uppercase << std::hex << std::setfill('0') << "PC:" << std::setw(4) << ip
               << " A:" << std::setw(2) << static_cast<unsigned>(a) << " X:" << std::setw(2) << static_cast<unsigned>(x)
               << " Y:" << std::setw(2) << static_cast<unsigned>(y) << " P:" << std::setw(2)
               << static_cast<unsigned>(sr) << " SP:" << std::setw(2) << static_cast<unsigned>(sp) << std::dec
               << " CYC:" << cycles;
        return output.str();
    }

    std::string format_instruction(const std::string& raw_line) {
        std::string instruction = raw_line.substr(0, raw_line.find("A:"));
        std::size_t last_character = instruction.find_last_not_of(" \t");
        if (last_character != std::string::npos) {
            instruction.erase(last_character + 1);
        }
        return instruction;
    }

    bool states_match(const ExpectedState& expected, const CPU::Registers& actual, uint64_t actual_cycles) {
        return expected.ip == actual.ip && expected.a == actual.a && expected.x == actual.x && expected.y == actual.y &&
               expected.sr == actual.sr && expected.sp == actual.sp && expected.cycles == actual_cycles;
    }

    void print_usage(const char* executable) {
        std::cerr << "Usage: " << executable << " [--all] <nestest.nes> <nestest.log>\n"
                  << "  --all  Compare unofficial opcodes too. By default, stop before the first unofficial opcode.\n";
    }
} // namespace

int main(int argc, char* argv[]) {
    bool include_unofficial = false;
    int path_argument = 1;

    if (argc > 1 && std::string(argv[1]) == "--all") {
        include_unofficial = true;
        path_argument++;
    }

    if (argc - path_argument != 2) {
        print_usage(argv[0]);
        return 2;
    }

    const std::string rom_path = argv[path_argument];
    const std::string log_path = argv[path_argument + 1];

    std::ifstream reference_log(log_path);
    if (!reference_log.is_open()) {
        std::cerr << "Unable to open reference log: " << log_path << '\n';
        return 2;
    }

    std::vector<ExpectedState> expected_states;
    std::string line;
    std::size_t source_line = 0;

    while (std::getline(reference_log, line)) {
        source_line++;
        if (line.empty()) {
            continue;
        }

        ExpectedState state;
        if (!parse_reference_line(line, source_line, state)) {
            std::cerr << "Unable to parse nestest.log line " << source_line << ":\n" << line << '\n';
            return 2;
        }

        expected_states.push_back(state);

        // Retain the first unofficial state as a sentinel. It validates the
        // result of the final official instruction without executing an
        // unofficial opcode.
        if (!include_unofficial && state.is_unofficial) {
            break;
        }
    }

    if (expected_states.empty()) {
        std::cerr << "The reference log contains no CPU states.\n";
        return 2;
    }

    Cartridge cartridge(rom_path);
    if (!cartridge.is_valid()) {
        std::cerr << "Unable to load nestest ROM: " << rom_path << '\n';
        return 2;
    }

    CPU cpu;
    cpu.set_cartridge(&cartridge);

    CPU::Registers initial_state{};
    initial_state.ip = 0xC000;
    initial_state.sp = 0xFD;
    initial_state.sr = 0x24;
    cpu.set_registers(initial_state);
    cpu.set_cycles(7);

    std::size_t executed_instructions = 0;

    for (std::size_t index = 0; index < expected_states.size(); index++) {
        const ExpectedState& expected = expected_states[index];
        const CPU::Registers& actual = cpu.get_registers();

        if (!states_match(expected, actual, cpu.get_cycles())) {
            std::cerr << "nestest mismatch at reference line " << expected.source_line << " after "
                      << executed_instructions << " instruction(s):\n"
                      << "  instruction: " << format_instruction(expected.raw_line) << '\n'
                      << "  expected: "
                      << format_state(
                             expected.ip,
                             expected.a,
                             expected.x,
                             expected.y,
                             expected.sr,
                             expected.sp,
                             expected.cycles
                         )
                      << '\n'
                      << "  actual:   "
                      << format_state(actual.ip, actual.a, actual.x, actual.y, actual.sr, actual.sp, cpu.get_cycles())
                      << '\n';
            return 1;
        }

        if (!include_unofficial && expected.is_unofficial) {
            break;
        }

        // The final full-log state has no following reference state against
        // which its result can be compared.
        if (index + 1 < expected_states.size()) {
            cpu.execute();
            executed_instructions++;
        }
    }

    if (!include_unofficial) {
        uint8_t official_result = cpu.cpu_read(0x0002);
        if (official_result != 0) {
            std::cerr << "Official nestest error code at $0002: $" << std::uppercase << std::hex << std::setw(2)
                      << std::setfill('0') << static_cast<unsigned>(official_result) << '\n';
            return 1;
        }
    }

    std::cout << "PASS: matched " << executed_instructions << (include_unofficial ? " nestest" : " official nestest")
              << " instruction state(s).\n";
    return 0;
}
