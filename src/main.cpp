#include "Emulator.h"
#include <iostream>
#include <fstream>
#include <filesystem>

//#define CONFIG_RUN_DIAGNOSTIC_MODE

int main()
{
    Emulator emulator;

#ifndef  CONFIG_RUN_DIAGNOSTIC_MODE

    emulator.loadROM("./roms/invaders.h", 0x0000);
    emulator.loadROM("./roms/invaders.g", 0x0800);
    emulator.loadROM("./roms/invaders.f", 0x1000);
    emulator.loadROM("./roms/invaders.e", 0x1800);

    if (emulator.boot("Intel 8080 Emulator", 4))
    {
        emulator.run();
    }

#else
    Bus& bus = emulator.getBus();
    Intel8080& cpu = emulator.getCPU();

    bus.romBoundary = 0;

    std::vector<std::string> romPaths =
    {
        "./roms/cpu_tests/TST8080.COM",
        "./roms/cpu_tests/CPUTEST.COM",
        "./roms/cpu_tests/8080PRE.COM",
        "./roms/cpu_tests/8080EXM.COM"
    };

    std::vector<size_t> expectedCycles =
    {
        4924L,
        255653383L,
        7817L,
        23803381171L
    };

    for (size_t i = 0; i < romPaths.size(); ++i)
    {
        std::string filePath = romPaths[i];
        std::string fileName = std::filesystem::path(filePath).filename().string();

        std::cout << ((i > 0) ? "\n" : "") << "********** Running test " << (i + 1) << "/" << romPaths.size() << ": " << fileName << "\n";

        for (int j = 0; j < 0x10000; j++) bus.mem.data[j] = 0x00;

        emulator.loadROM(filePath.c_str(), 0x0100);

        bus.mem.data[0x0000] = 0xD3; // OUT instruction
        bus.mem.data[0x0001] = 0x00; // Port 0 (System Exit)

        bus.mem.data[0x0005] = 0xC9; // <--- ADD THIS: RET instruction

        bus.mem.data[0x0006] = 0x00; // Low Byte of RAM size
        bus.mem.data[0x0007] = 0xF0; // High Byte of RAM size (0xF000)

        cpu.PC = 0x0100;
        cpu.SP = 0xF000;

        size_t instruction_n = 0;
        size_t cycles_n = 0;

        while (true)
        {
            if (cpu.PC == 0x0005)
            {
                if (cpu.regs.C == 0) // CP/M Function 0: System Reset / Clean Exit
                {
                    cycles_n += 10;
                    instruction_n += 1;
                    break;
                }
                else if (cpu.regs.C == 9) // Function 9: Print String Block
                {
                    uint16_t strAddr = cpu.regs.DE;
                    while (bus.readMemory(strAddr) != '$')
                    {
                        std::cout << static_cast<char>(bus.readMemory(strAddr++));
                    }
                    std::cout << std::flush;
                }
                else if (cpu.regs.C == 2) // Function 2: Print Character
                {
                    std::cout << static_cast<char>(cpu.regs.E) << std::flush;
                }

                // Standard rigs execute an OUT then a RET here (2 inst, 20 cycles)
                cycles_n += 20;
                instruction_n += 2;

                cpu.PC = cpu.pop16(); // Return to test execution
                continue; // Skip the physical CPU step
            }

            if (cpu.PC == 0x0000)
            {
                // Standard rigs execute an exit OUT instruction here
                cycles_n += 10;
                instruction_n += 1;
                break;
            }

            // Run the step
            cycles_n += cpu.step();
            instruction_n++;
        }
        std::cout << "\n****************************************\n";
    }

#endif

    return 0;
}
