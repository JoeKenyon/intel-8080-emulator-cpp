#pragma once
#include <SDL2/SDL.h>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

class MachineBus; // forward declaration

// A single binary blob to be loaded into memory at boot.
// Space Invaders ships as 4 separate PROM dumps (h/g/f/e), other boards
// might ship as one file or a dozen - just add one RomImage per chunk.
struct RomImage
{
    std::string path;
    uint16_t    loadAddress;
};

// Maps a physical key to a bit in an input port. Held down = bit set.
struct KeyBinding
{
    SDL_Keycode key;
    uint8_t     port;
    uint8_t     mask;
};

// Most 8080 arcade boards raise one or more periodic interrupts per video
// frame (Space Invaders raises two: mid-screen and end-of-screen, to reduce
// visible tearing). cyclesIntoFrame is measured from the start of the frame.
struct FrameInterrupt
{
    long    cyclesIntoFrame;
    uint8_t vector; // RST vector, e.g. 0x08 = RST 1, 0x10 = RST 2
};

// Describes everything the emulator core needs to know about a specific
// machine/game. Build one of these per ROM set and 
// hand it to Emulator - nothing else needs to change.
struct MachineConfig
{
    std::string name = "Unnamed Machine";

    // --- Memory ---
    std::vector<RomImage> roms;
    uint16_t writableFrom = 0x2000; // addresses below this are read-only ROM

    // --- Video ---
    int      screenWidth  = 224;
    int      screenHeight = 256;
    int      windowScale  = 3;
    uint16_t vramStart     = 0x2400; // 1bpp framebuffer, column-major, MSB = bottom pixel

    // --- Timing ---
    long cyclesPerSecond = 2'000'000;
    int  framesPerSecond = 60;
    std::vector<FrameInterrupt> interrupts;

    // --- Input ---
    std::vector<KeyBinding> keyBindings;

    // Port bits tied high/low on the physical chassis wiring (dip switches,
    // unused inputs, tilt sensors, etc). Applied every frame before polling input.
    std::vector<std::pair<uint8_t, uint8_t>> staticPortBitsSet;   // (port, mask) forced to 1
    std::vector<std::pair<uint8_t, uint8_t>> staticPortBitsClear; // (port, mask) forced to 0

    // --- Custom hardware ---
    // Anything beyond simple input ports (shift registers, sound boards, etc).
    // Leave null to fall back to plain port-array reads/writes on MachineBus.
    std::function<uint8_t(MachineBus&, uint8_t port)>          customPortRead;
    std::function<void(MachineBus&, uint8_t port, uint8_t val)> customPortWrite;
};