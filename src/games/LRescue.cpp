#include "LRescue.h"
#include "../core/MachineBus.h"
#include <memory>
#include <string>

namespace LunarRescue
{
    // the shift register lives on ports 2-4 and needs to persist between the
    // read and write hooks below, so it's captured by shared_ptr rather than
    // being a plain local/static. keeps the config instance self-contained.
    struct ShiftRegisterHardware
    {
        uint16_t shiftRegister = 0;
        uint8_t  shiftOffset   = 0;
    };

    MachineConfig buildConfig()
    {
        MachineConfig config;

        config.name = "LunarRescue";

        config.roms =
        {
            {"./roms/lrescue/lrescue.1", 0x0000},
            {"./roms/lrescue/lrescue.2", 0x0800},
            {"./roms/lrescue/lrescue.3", 0x1000},
            {"./roms/lrescue/lrescue.4", 0x1800},

            // lunar rescue maps its extra 4kb of code up at 0x4000
            {"./roms/lrescue/lrescue.5", 0x4000},
            {"./roms/lrescue/lrescue.6", 0x4800}
        };

        config.writableFrom = 0x2000;

        config.screenWidth  = 224;
        config.screenHeight = 256;
        config.windowScale  = 3;
        config.vramStart    = 0x2400;

        config.cyclesPerSecond = 2'000'000;
        config.framesPerSecond = 60;

        config.interrupts =
        {
            {16'666, 0x08}, // mid-screen (rst 1)
            {33'332, 0x10}, // end-of-screen (rst 2)
        };

        config.keyBindings =
        {
            {SDLK_c,      1, 0b0000'0001}, // insert coin
            {SDLK_RETURN, 1, 0b0000'0100}, // player 1 start
            {SDLK_SPACE,  1, 0b0001'0000}, // player 1 shoot
            {SDLK_LEFT,   1, 0b0010'0000}, // player 1 move left
            {SDLK_RIGHT,  1, 0b0100'0000}, // player 1 move right
        };

        // bits tied high/low by the chassis wiring loom
        config.staticPortBitsSet   = { {1, 0x08}, {2, 0x40} }; // port1 bit3 always high; coin-door loop
        config.staticPortBitsClear = { {2, 0x04} };            // tilt sensor off

        // heap allocate the shift register so the callbacks can safely capture and share it
        auto hw = std::make_shared<ShiftRegisterHardware>();

        config.customPortRead = [hw](MachineBus& bus, uint8_t port) -> uint8_t
        {
            switch (port)
            {
                case 1:
                    return bus.ports[1];
                case 2:
                    return bus.ports[2];
                case 3:
                    return (hw->shiftRegister >> (8 - hw->shiftOffset)) & 0xFF;
                default:
                    return 0;
            }
        };

        config.customPortWrite = [hw](MachineBus&, uint8_t port, uint8_t val)
        {
            switch (port)
            {
                case 2:
                    hw->shiftOffset = val & 0x07;
                    break;
                case 4:
                    hw->shiftRegister = (hw->shiftRegister >> 8) | (static_cast<uint16_t>(val) << 8);
                    break;
                default:
                    break;
            }
        };

        return config;
    }
}
