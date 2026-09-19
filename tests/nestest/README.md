# nestest CPU harness

This harness runs the CPU in nestest automation mode and compares its state with `nestest.log` before each
instruction. It compares `PC`, `A`, `X`, `Y`, `P`, `SP`, and CPU cycle count, while intentionally ignoring the
reference log's disassembly and PPU position.

Place these locally downloaded files in `tests/nestest/artifacts/`:

- `nestest.nes`
- `nestest.log`
- `nestest.txt` (documentation only; the runner does not read it)

The `artifacts` directory is ignored by Git.

## Build and run

From the repository root:

```text
cmake --build build --config Debug --target NES_Nestest
.\tests\nestest\run_nestest.cmd
```

The default run starts with the canonical automation state (`PC=$C000`, `A=X=Y=$00`, `P=$24`, `SP=$FD`,
`CYC=7`) and stops before executing the first unofficial opcode. The first unofficial log entry remains as a
sentinel so the final official instruction's result is still checked.

To compare the complete log after unofficial opcodes are implemented:

```text
.\tests\nestest\run_nestest.cmd --all
```

The runner exits on the first mismatch and prints the reference line alongside the emulator's actual state.
