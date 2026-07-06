# Intel 8080 Emulator (Space Invaders)

A cycle-accurate Intel 8080 CPU emulator written in C++20, targeting Space
Invaders as the primary test ROM, with SDL2 for video and input.

## Features

- Full 8080 instruction set via a function-pointer dispatch table (`OPCODE_TABLE`)
- Real flag computation (Zero, Sign, Parity, Carry, Aux Carry) with hardware-accurate
  half-carry/borrow logic for ALU ops
- Interrupt support (`RST` vector servicing, enable/disable, halt)
- Space Invaders hardware: VRAM rendering, bit-shift register I/O (ports 2/3/4),
  keyboard-mapped input ports
- CP/M BDOS intercept mode for running the standard 8080 CPU test suite
  (`TST8080`, `CPUTEST`, `8080PRE`, `8080EXM`)

## Build

Requires CMake ≥ 3.15 and a C++20 compiler. SDL2 is fetched automatically via
`FetchContent`.

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Place Space Invaders ROM files (`invaders.h`, `.g`, `.f`, `.e`) in `roms/` at
the project root before running.

```bash
./intel-8080-emulator-cpp
```

## Controls

| Key       | Action           |
|-----------|------------------|
| C         | Insert coin      |
| Enter     | Player 1 start   |
| Space     | Fire             |
| Left/Right| Move             |
| Esc       | Quit             |

## Architecture

```
Emulator          owns Bus, Intel8080, Display; runs the 60Hz frame loop
├── Bus           64KB memory + 256 I/O ports; Space Invaders shift register
│   └── Memory    raw byte array read/write
├── Intel8080     registers, flags, interrupt state, step()/dispatch
│   └── OPCODE_TABLE   256-entry table of { mnemonic, cycles, handler }
└── Display       SDL2 window/renderer, VRAM → texture, keyboard → ports
```

CPU logic is split across:
- `Intel8080.cpp` — core fetch/decode/dispatch loop, register decode helper
- `Opcodes_ALU.cpp` — arithmetic/logic instructions and flag helpers
- `Opcodes_Move.cpp` — data movement, stack, register pair ops
- `Opcodes_System.cpp` — jumps, calls, returns, RST, interrupts, I/O

Each opcode handler is a single member function driven by bitfields decoded
from the raw opcode byte at runtime (e.g. `op_MOV`, `op_AluReg`), rather than
one function per opcode.

## CPU test mode

Build with `CONFIG_RUN_TEST_MODE` defined to run the classic 8080 diagnostic
ROMs (`TST8080.COM`, `CPUTEST.COM`, `8080PRE.COM`, `8080EXM.COM`) through a
CP/M BDOS-intercept harness instead of booting the Space Invaders hardware.
Output is printed to stdout for verification.

## Status / known gaps

- I/O ports are currently handled via a hardcoded switch in `Bus`; a
  per-port handler table is a planned cleanup (see below).
- Memory access is routed through `Bus`, but a full `Bus` abstraction
  (for supporting machines beyond Space Invaders) is not yet in place.

## Roadmap

- [ ] Replace port switch statements with per-port handler tables
- [ ] Additional ROM target support
