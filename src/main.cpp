#include "Emulator.h"

//#define CONFIG_RUN_TEST_MODE
#ifndef CONFIG_RUN_TEST_MODE

int main()
{
    Emulator emulator;
    emulator.loadROM("./roms/invaders.h", 0x0000);
    emulator.loadROM("./roms/invaders.g", 0x0800);
    emulator.loadROM("./roms/invaders.f", 0x1000);
    emulator.loadROM("./roms/invaders.e", 0x1800);

    if (emulator.boot("Intel 8080 Emulator", 3))
    {
        emulator.run();
    }
    return 0;
}

#else

#include <iostream>
#include <filesystem>

int main()
{
    Emulator emulator;
    Bus& bus = emulator.getBus();
    Intel8080& cpu = emulator.getCPU();
    bus.romBoundary = 0;

    bool testFinished = false;

    // Port 0 OUT: the test ROMs execute this as their "done" signal.
    bus.setOutHandler(0, [&testFinished](uint8_t)
    {
        testFinished = true;
    });

    // Port 1 OUT: stands in for the CP/M BDOS print calls. The CPU actually
    // executes real CALL 0x0005 / OUT 1,A / RET instructions — we just read
    // whatever the ROM left in C/D/E to decide what to print, same as a real
    // BDOS would.
    bus.setOutHandler(1, [&cpu, &bus](uint8_t)
    {
        uint8_t operation = cpu.regs.C;

        if (operation == 2) // Print Character (register E)
        {
            std::cout << static_cast<char>(cpu.regs.E);
        }
        else if (operation == 9) // Print String Block ('$'-terminated, at DE)
        {
            uint16_t addr = cpu.regs.DE;
            do
            {
                std::cout << static_cast<char>(bus.readMemory(addr++));
            } while (bus.readMemory(addr) != '$');
        }

        std::cout << std::flush;
    });

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

        bus.mem.data[0x0005] = 0xD3; // OUT instruction
        bus.mem.data[0x0006] = 0x01; // Port 1 (BDOS print call)
        bus.mem.data[0x0007] = 0xC9; // RET, back into the test ROM

        cpu.PC = 0x0100;
        cpu.SP = 0xF000;

        size_t instruction_n = 0;
        size_t cycles_n = 0;
        testFinished = false;

        while (!testFinished)
        {
            cycles_n += cpu.step();
            instruction_n++;
        }

        long long diff = static_cast<long long>(expectedCycles[i]) - static_cast<long long>(cycles_n);
        std::cout << "\n*** " << instruction_n << " instructions executed on " << cycles_n
                   << " cycles (expected=" << expectedCycles[i] << ", diff=" << diff << ")\n";
        std::cout << "****************************************\n";
    }

    return 0;
}
#endif
