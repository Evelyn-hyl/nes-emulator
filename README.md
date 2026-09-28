# Etalume - NES Emulator (C++/SDL2)

Etalume is a work-in-progress NES emulator written in C++20, with SDL2 for windowing and framebuffer display. The current focus is NROM (mapper 0): building a tested CPU core, rendering NES backgrounds, and integrating CPU/PPU timing into a running emulator.

The project now includes all official 6502 opcodes, a headless nestest trace harness, iNES cartridge loading, a per-cycle PPU background pipeline, and an SDL2 frontend. Sprite rendering and controller input are still unfinished, so this is not yet a fully playable emulator.

## Demos

Images / Videos will be added here as development progresses.

### CHR pattern-table rendering

An early graphics milestone: decoding cartridge CHR data into tiles and displaying it through SDL2.

<img width="511" height="542" alt="Screenshot 2026-08-18 225929" src="https://github.com/user-attachments/assets/175a34c4-e09a-4847-9400-dc94ae97b885" />


### Background rendering and CPU/PPU integration

Show the current background output, scrolling, and ROM execution. Rendering and timing are still being refined.

https://github.com/user-attachments/assets/c45caf13-f03a-4afb-a119-e829e14cf93a

### CPU validation with nestest



## Progress so far

This overview includes committed milestones and the current local integration work. Implemented features are not a claim of full hardware accuracy or broad game compatibility.

| Area | Progress |
| --- | --- |
| CPU | All official 6502 opcodes and addressing modes implemented, including arithmetic, logic, branches, stack operations, jumps, and instruction cycle accounting. Reset-vector handling is present. |
| CPU validation | Headless nestest comparison harness added; the official instruction trace pass is recorded in commit `6954626`. Fixes include ADC/SBC flags, indexed read-modify-write addressing, shifts, indirect JMP wrapping, and JSR return addresses. |
| Cartridge / mapper | iNES loading, mapper 0 PRG-ROM mapping for 16 KB and 32 KB cartridges, CHR-ROM reads, and CHR-RAM writes. |
| Bus | CPU/cartridge/PPU connections, 2 KB CPU RAM and its mirrors, mirrored PPU registers, and PRG-ROM reads. Local integration work expands the address routing and initialization. |
| PPU registers / memory | Register reads and writes, buffered PPUDATA reads, open-bus behavior, scroll/address latches, horizontal/vertical nametable mirroring, palette RAM, and OAM register access. |
| Background graphics | CHR pattern-table visualization, nametable/attribute/pattern fetches, background shift registers, palette lookup, scroll updates, and a 256 x 240 framebuffer. |
| PPU timing | Per-cycle clocking, scanline/frame progression, VBlank set/clear behavior, and odd-frame handling. Timing accuracy is still being refined. |
| SDL2 frontend | Window and streaming texture display at 4x scale, close-window event handling, and a frame limiter targeting approximately 60.1 FPS. |
| Integration / diagnostics | Current local work connects ROM execution to the frame loop at three PPU clocks per elapsed CPU cycle, adds VBlank NMI delivery at instruction boundaries, and provides optional CPU/PPU tracing. |
| Build / tooling | CMake targets for the emulator and nestest runner, SDL2 discovery with a FetchContent fallback, shared types/header cleanup, and clang-format configuration with a tracked pre-commit hook. |

## Contributors

Contributions below are based on the repository's commit history. Both contributors have worked on the core; the original CPU/PPU split has evolved as integration progressed.

| Contributor | Work completed |
| --- | --- |
| **Evelyn-hyl** | PPU register and memory behavior; scroll/address handling and mirroring; CHR visualization and background rendering pipeline; per-cycle PPU and VBlank timing; iNES cartridge loader and CHR-RAM fixes; SDL2 render loop and frame limiter; completion of official CPU opcode coverage and CPU correctness fixes; nestest harness and official-trace pass; CMake, formatting hooks, shared-header cleanup, and integration fixes. |
| **NM711** | Initial project structure; CPU status helpers and ADC implementation; CMP, CPX, CPY, DEC, DEX, DEY, EOR, BIT, branch, and AND instructions with their addressing modes; initial bus skeleton and CPU/PPU communication; mapper 0 implementation. |

Uncommitted integration and tracing changes are included in the progress overview but are not assigned to an author from Git history.

## Remaining work

- **Sprites:** finish evaluation, pattern fetching, and composition with backgrounds; implement sprite-zero hit and overflow behavior. OAM storage and pipeline scaffolding exist, but sprites are not rendered yet.
- **OAM DMA:** route `$4014` transfers and account for CPU stalls.
- **Controller input:** implement keyboard mapping and controller reads/writes at `$4016` / `$4017`.
- **Timing and interrupts:** refine CPU/PPU synchronization and NMI behavior, add IRQ handling, and validate timing edge cases. The current loop advances the PPU after each CPU instruction or interrupt.
- **Testing:** expand beyond the official nestest trace to CPU/PPU test ROMs and documented game compatibility checks. Unofficial opcodes are not covered by the default harness run.
- **Debugger:** add interactive stepping, breakpoints, register/disassembly views, and memory/PPU inspectors. Trace logging is the current diagnostic tool.

Audio/APU emulation, additional mappers, save states, rewind, and netplay remain outside the initial MVP scope.

## Build

Requirements:

- A C++20 compiler and CMake 3.16 or newer.
- SDL2 development libraries, or Git and network access for CMake to fetch SDL2 when no local installation is found.

From the repository root:

```sh
cmake -S . -B build
cmake --build build --config Debug
```

CMake builds `NES_Emulator` and `NES_Nestest`. It also configures the repository's tracked Git hooks when possible; the pre-commit hook uses clang-format.

## Run

Supply a mapper 0 ROM in iNES format. ROMs are not bundled.

For a Windows multi-configuration build:

```powershell
.\build\Debug\NES_Emulator.exe "path\to\game.nes"
```

For a single-configuration build (for example, Makefiles or Ninja):

```sh
./build/NES_Emulator path/to/game.nes
```

Without a ROM argument, the application looks for `smb.nes` in the current working directory. Close the window to exit; keyboard gameplay controls are not implemented yet.

The current local integration work also supports tracing:

```powershell
.\build\Debug\NES_Emulator.exe "path\to\game.nes" --trace
```

This writes CPU instruction state, PPU register accesses, and NMI/VBlank events to `nes_trace.log` in the working directory, replacing any previous trace.

## CPU testing

Place `nestest.nes` and its reference `nestest.log` in `tests/nestest/artifacts/` (ignored by Git), then run:

```powershell
cmake --build build --config Debug --target NES_Nestest
.\tests\nestest\run_nestest.cmd
```

The Windows helper expects `build/Debug/NES_Nestest.exe`. For other build layouts, invoke the runner directly:

```sh
./build/NES_Nestest tests/nestest/artifacts/nestest.nes tests/nestest/artifacts/nestest.log
```

The runner compares `PC`, `A`, `X`, `Y`, `P`, `SP`, and CPU cycle count before each instruction and reports the first mismatch. By default, it checks the official-opcode portion of the trace, retaining the first unofficial entry as a sentinel to verify the final official instruction. It does not validate PPU timing or rendering.

See [the nestest harness documentation](tests/nestest/README.md) for initial CPU state and the optional `--all` mode for future unofficial-opcode testing.

## Code layout

| Path | Purpose |
| --- | --- |
| `include/` | CPU, PPU, bus, cartridge, mapper, and timing interfaces and shared types. |
| `src/cpu/` | CPU instruction execution, state, and cycle accounting. |
| `src/ppu/` | PPU registers, memory, background rendering, and timing. |
| `src/bus/` | CPU memory map and component routing. |
| `src/cartridge/` | iNES loading and mapper implementation. |
| `src/main.cpp` | Application setup, CPU/PPU execution loop, and SDL2 display. |
| `src/ui/` | Frame limiter and window-module placeholder. |
| `src/input/` | Controller-module placeholder. |
| `tests/nestest/` | Headless CPU trace runner and Windows launch script. |

## Commit format

`<type>(<scope>): <summary>`

Types: `feat`, `fix`, `refactor`, `perf`, `test`, `docs`, `build`, `chore`.
Scopes: `cpu`, `ppu`, `apu`, `disassembler`, `mapper`, `nes`, `util`, `build`.

Example: `feat(cpu): implement ADC/SBC with overflow flag handling`
