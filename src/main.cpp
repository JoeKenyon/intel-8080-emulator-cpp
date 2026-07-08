#include "Emulator.h"
#include <iostream>


#include "CPU.h"
#include <fstream>
#include <iostream>
#include <functional>
#include <array>

class TestBus : public CPU::IBus
{
    uint8_t memory[0x10000] = {0};

public:
    using OutHandler = std::function<void(uint8_t)>;
    using InHandler  = std::function<uint8_t()>;

    std::array<OutHandler, 256> outHandlers{};
    std::array<InHandler, 256>  inHandlers{};

    uint8_t readByte(uint16_t addr) override { return memory[addr]; }
    void writeByte(uint16_t addr, uint8_t val) override { memory[addr] = val; }

    uint8_t readPort(uint8_t port) override { if (inHandlers[port]) return inHandlers[port](); return 0; }
    void writePort(uint8_t port, uint8_t val) override { if (outHandlers[port]) outHandlers[port](val); }

    bool loadROM(const char* filepath, uint16_t offset) noexcept
    {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            std::cerr << "Failed to locate rom: " << filepath << "\n";
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (!file.read(reinterpret_cast<char*>(&memory[offset]), size))
        {
            return false;
        }
        return true;
    }
};

int main()
{
    TestBus bus;
    CPU cpu(bus);

    bool testFinished = false;

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

    bus.outHandlers[0] = [&testFinished](uint8_t)
    {
        testFinished = true;
    };

    bus.outHandlers[1] = [&cpu, &bus](uint8_t)
    {
        if (cpu.C == 2)
        {
            std::cout << static_cast<char>(cpu.E);
        }
        else if (cpu.C == 9)
        {
            uint16_t addr = cpu.DE;
            while (bus.readByte(addr) != '$')
            {
                std::cout << static_cast<char>(bus.readByte(addr++));
            }
        }
    };

    for (size_t i = 0; i < romPaths.size(); ++i)
    {
        std::string filePath = romPaths[i];
        std::string fileName = std::filesystem::path(filePath).filename().string();

        std::cout << ((i > 0) ? "\n" : "") << "********** Running test " << (i + 1) << "/" << romPaths.size() << ": " << fileName << "\n";

        for (int j = 0; j < 0x10000; j++) bus.writeByte(j, 0x0);

        if (!bus.loadROM(filePath.c_str(), 0x0100))
            return 1;

        bus.writeByte(0x0000,0xD3); // OUT instruction
        bus.writeByte(0x0001,0x00); // Port 0 (System Exit)

        bus.writeByte(0x0005,0xD3); // OUT instruction
        bus.writeByte(0x0006,0x01); // Port 1 (BDOS print call)
        bus.writeByte(0x0007,0xC9); // RET, back into the test ROM

        cpu.PC = 0x0100;
        cpu.SP = 0xF000;

        size_t instruction_n = 0;
        size_t cycles_n = 0;
        testFinished = false;

        while (!testFinished)
        {
            cycles_n += cpu.executeInstruction();
            instruction_n++;
        }

        long long diff = static_cast<long long>(expectedCycles[i]) - static_cast<long long>(cycles_n);
        std::cout << "\n*** " << instruction_n << " instructions executed on " << cycles_n
                   << " cycles (expected=" << expectedCycles[i] << ", diff=" << diff << ")\n";
        std::cout << "****************************************\n";
    }
    return 0;
}
