# Etalume - NES Emulator (C++/SDL2)

A minimal but functional NES emulator built in C++ using SDL2 for windowing/input/rendering.
Goal: run real NROM (mapper 0) games/test ROMs with correct CPU behavior and a serviceable,
naively-implemented PPU. Timeline: **2–3 weeks**, 2 contributors.

## Scope

### In scope (MVP)
- MOS 6502 CPU core (all official opcodes, correct cycle timing, interrupts: NMI/IRQ/RESET)
- CPU/PPU/Memory bus, RAM mirroring, iNES ROM loader
- Mapper 0 (NROM) only
- Naive PPU: background tile rendering + sprite rendering (8x8 only is fine to start,
  8x16 as stretch), no scanline-accurate tricks, no mid-frame raster effects required
- SDL2 frontend: window, framebuffer blit, keyboard-mapped controller input
- Step/instruction-level debugger (registers, memory viewer, disassembly, breakpoints)
- Test ROM suite integration (nestest, blargg's CPU/PPU test ROMs)

### Explicitly out of scope
- APU / audio (no sound at all)
- Scanline-accurate PPU timing, sprite 0 hit edge cases, sprite overflow flag accuracy
- Mappers beyond NROM (MMC1/MMC3 etc. are stretch/post-MVP)
- Save states, rewind, netplay, GUI polish

## Team split

Because the CPU and PPU/frontend are both large but fairly separable, each week both
people touch related-but-distinct pieces rather than working in totally separate silos.
This keeps integration pain low (you're merging daily, not at the end of a week).

- **Person A — "CPU/Core"**: 6502 instruction set, bus/memory, interrupts, mapper 0,
  ROM loading, debugger backend.
- **Person B — "PPU/Frontend"**: SDL2 shell, PPU registers + rendering, input,
  framebuffer, debugger UI/visualization.

You will both touch the CPU↔PPU timing glue and the test harness — that's the
integration seam and it's worth pairing on.

---

## Week 1 — Core skeleton, both halves running in parallel

**Shared (Day 1, do together):**
- Repo setup, CMake + SDL2 build working on all your machines
- Agree on the Bus interface (`read8/write8`, memory map constants, PPU register
  addresses `$2000-$2007`, mirroring rules) — write this as a header both branch off of
- Pick and vendor a starting test ROM: `nestest.nes`

**Person A (CPU/Core):**
- Implement CPU registers, flags, addressing modes
- Implement official opcodes in groups (loads/stores → arithmetic/logic → branches/jumps
  → stack/misc) with correct cycle counts
- Implement RESET vector handling, get CPU running against `nestest.nes` in
  "no PPU" / headless mode, diff output against the known-good `nestest.log`

**Person B (PPU/Frontend):**
- SDL2 window + framebuffer (just push a static/test pattern to prove the pipeline)
- iNES header parser + cartridge/mapper-0 PRG/CHR loading
- PPU memory map skeleton: pattern tables, nametables, palette RAM, OAM — registers
  stubbed (reads/writes land somewhere, no rendering logic yet)
- Keyboard input polling wired to a controller struct (not yet connected to CPU reads)

**End of week 1 checkpoint:** CPU passes nestest (or you know exactly which opcodes are
still wrong via log diff), and an SDL2 window opens and can draw an arbitrary framebuffer.

---

## Week 2 — Make it render, wire it together

**Person A (CPU/Core):**
- Finish any remaining opcodes/edge cases from nestest diff
- Implement NMI (triggered by PPU vblank) and IRQ handling
- Implement CPU-side `$4014` OAM DMA
- Start the debugger backend: expose register state, step-one-instruction,
  breakpoint list, memory read for a given address range

**Person B (PPU/Frontend):**
- Implement background rendering: nametable → pattern table → palette lookup →
  framebuffer, scroll registers (`$2005`/`$2006`) at a basic (per-frame, not per-scanline)
  granularity
- Implement sprite rendering from OAM (naive: no sprite 0 hit, no 8-sprite-per-line limit
  needed for MVP, but easy to add if time allows)
- Wire controller reads to `$4016`/`$4017`
- Basic PPU register behavior: vblank flag set/clear, `$2002` read side effects

**Shared (end of week, pair session):**
- Connect CPU and PPU on a shared clock (simplest correct approach: run PPU 3 ticks per
  1 CPU cycle, catch up PPU before any CPU read/write that touches PPU state)
- Get a real, simple game or test ROM showing a stable image

**End of week 2 checkpoint:** A real ROM boots and renders something recognizable on
screen with input working.

---

## Week 3 — Debugger, test suite, bug fixing buffer

**Person A (CPU/Core):**
- Finish debugger backend: breakpoints on PC/address access, step-over, step-into
- Run blargg's CPU test ROMs, fix any remaining CPU accuracy bugs
- Write a small automated test harness (script that runs test ROMs headless and checks
  known success bytes/output, so you're not eyeballing every regression)

**Person B (PPU/Frontend):**
- Debugger UI: register/flag display, disassembly view, memory hex viewer, pattern
  table/nametable/palette viewers (extremely useful for catching PPU bugs visually)
- Run blargg's PPU test ROMs, fix rendering bugs they surface
- Polish: pause/step/reset controls, FPS cap/vsync so games run at correct speed

**Shared (last 2–3 days):**
- Integration bug bash: play several real ROMs, log/fix crashes and glitches
- README/usage docs, build instructions, controls
- Stretch goals if time remains (pick based on what's most broken/interesting):
  - Second mapper (MMC1)
  - 8x16 sprites
  - Save states
  - Simple APU (square wave only)

**End of week 3 checkpoint:** MVP demo — a real commercial or homebrew NROM game
running at correct speed with working input and a usable debugger.

---

## Compressing to 2 weeks

If you only have 2 weeks, merge Week 1 and Week 2: skip the "headless nestest only"
milestone and get CPU+PPU integrated by end of week 1 even if buggy, then spend all of
week 2 on the debugger + blargg test suite + bug fixing. You lose the clean checkpoint
but the deliverable is the same.

## Test ROM suite

- `nestest.nes` — CPU correctness (has a well-known expected log to diff against)
- blargg's `cpu_dummy_reads`, `instr_test-v5`, `nmi_sync`, `ppu_vbl_nmi`,
  `sprite_hit_tests`, `oam_read`/`oam_stress` — pull the ones relevant to your scope
- 1–2 real early NROM games (e.g. *Donkey Kong*, *Balloon Fight*, *Ice Climber*) as
  end-to-end smoke tests

## Build Requirements

Requires SDL2 development libraries installed (`libsdl2-dev` on Debian/Ubuntu,
`sdl2` via Homebrew on macOS, or vcpkg/SDL2 dev package on Windows).

## References

- NESdev Wiki — https://www.nesdev.org/wiki/Nesdev_Wiki (the single best resource for
  CPU opcode tables, PPU register behavior, and the iNES format)
- `nestest.nes` and its reference log (widely mirrored on GitHub)
- blargg's NES test ROM collections (search "blargg nes tests" on GitHub)

## Commit format

`<type>(<scope>): <summary>` — types: `feat`, `fix`, `refactor`, `perf`, `test`,
`docs`, `build`, `chore`. Scopes: `cpu`, `ppu`, `apu`, `disassembler`, `mapper`,
`nes`, `util`, `build`.

Example: `feat(cpu): implement ADC/SBC with overflow flag handling`
