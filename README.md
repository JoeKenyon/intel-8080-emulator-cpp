# Taito 8080 Emulator

A cycle-accurate Intel 8080 emulator in C++20, targeting Taito 8080 Hardware,
the arcade board Taito developed in 1977, most famous for Space Invaders and
also used by later derivatives like Lunar Rescue. SDL2 handles video and
input.

<img src="space_invaders.png" alt="running space invaders" width="350"/>

This isn't a single game emulator. The core (CPU, bus, display) is shared
across the whole board family, and each game gets added as a data driven
config rather than hardcoded into the emulator itself.

## Currently supported

- Space Invaders (Taito/Midway, 1978)
- Lunar Rescue (Taito, 1979)

## Features

- Full 8080 instruction set via a 256 entry dispatch table (`OPCODE_TABLE`),
  each entry storing mnemonic, cycle cost, and handler pointer
- Real flag computation (Zero, Sign, Parity, Carry, Aux Carry) with
  hardware accurate half carry/borrow logic for ALU ops
- Interrupt support (RST vector servicing, enable/disable, halt)
- Data driven `MachineConfig` describing ROM layout, VRAM/screen info,
  frame timing and interrupts, key bindings, static port bits, and any
  custom hardware hooks per game
- Shift register hardware emulated for both supported games (ports 2/3/4),
  matching the board's bit shifter used for fast sprite blits
- A standalone CPU correctness harness validated against the standard
  8080 diagnostic ROMs (`TST8080`, `CPUTEST`, `8080PRE`, `8080EXM`), plus
  differential fuzzing against a second independent 8080 implementation
  with zero flag mismatches

## Build

Requires CMake >= 3.15 and a C++20 compiler. SDL2 is fetched automatically
via `FetchContent`.

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Drop the relevant ROM files into `roms/<game>/` before running, e.g.
`roms/invaders/invaders.h/.g/.f/.e` or `roms/lrescue/lrescue.1` through
`.6`. Pick which game to boot by pointing `main.cpp` at the right
`buildConfig()` (see Adding a new game below for how this works).

```bash
./taito8080-emu
```

## Controls

| Key        | Action           |
|------------|------------------|
| C          | Insert coin      |
| Enter      | Player 1 start   |
| Space      | Fire             |
| Left/Right | Move             |
| Esc        | Quit             |

## Architecture

```
Emulator          owns MachineBus, CPU, Display; runs the frame loop
├── MachineBus    64KB memory + 256 I/O ports, implements CPU::IBus
│                 routes custom port reads/writes to MachineConfig hooks
├── CPU           registers, flags, interrupt state, executeInstruction()
│   └── OPCODE_TABLE   256 entry table of { mnemonic, cycles, handler }
├── MachineConfig  per game data: ROMs, screen/VRAM, timing, interrupts,
│                 key bindings, static port bits, custom hardware callbacks
└── Display       SDL2 window/renderer, VRAM to texture, keyboard to ports
```

`CPU` only talks to memory and I/O through the abstract `IBus` interface,
so it has zero knowledge of SDL2, ROM layout, or which game is running.
`MachineBus` is the concrete `IBus` for arcade hardware, and it defers
anything machine specific (like the shift register) to the callbacks set
on `MachineConfig`.

### Why MachineConfig instead of just hardcoding Space Invaders?

"Taito 8080" describes a hardware generation, not one fixed board. Even
between the two games currently supported, ROM layout, shift register
wiring, and static port bits (dip switches, coin door, tilt sensor) all
differ. Hardcoding one game would make the emulator's name a lie the
moment a second game got added. `MachineConfig` is the boundary between
what's actually constant across the family (CPU, bus protocol, display
pipeline) and what varies per board (memory map, peripherals, controls).

## CPU correctness

The core is checked against the standard 8080 diagnostic suite:

- `TST8080.COM`
- `CPUTEST.COM`
- `8080PRE.COM`
- `8080EXM.COM`

Cycle counts are compared against known good totals for each ROM. The ALU
has also been differentially fuzzed against a second, independent 8080
implementation ([superzazu/8080](https://github.com/superzazu/8080)) with
zero flag mismatches found.

<img src="testing.png" alt="test results" width="450"/>

## Adding a new game

1. Write a new header (e.g. `MyGame.h`) with a `buildConfig()` function
   returning a filled out `MachineConfig`.
2. Fill in ROM images/addresses, screen/VRAM info, timing, interrupts,
   key bindings, and static port bits.
3. If the board has custom hardware beyond simple port bits (shift
   register, sound board, etc), implement `customPortRead`/`customPortWrite`.
4. Point `main.cpp` at the new config.

No changes to `CPU`, `MachineBus`, `Display`, or `Emulator` should be
needed unless the new board introduces something genuinely different,
like a second CPU, non-bitmap video, or analog/positional input. At that
point it's probably its own project rather than a `MachineConfig` entry
here.

## Roadmap

- [ ] Add another game on the existing hardware model (Balloon Bomber or
      Ozma Wars are the easiest next targets)
- [ ] Gun Fight support (would require extending `MachineConfig` for
      analog/positional input and no shift register)