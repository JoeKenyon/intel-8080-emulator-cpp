#include "../core/CPU.h"
#include "../ui/font8x8_basic.h"
#include "CpuDiagnostics.h"

#include <SDL2/SDL.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

namespace
{
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
                return false;
            }

            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            return static_cast<bool>(file.read(reinterpret_cast<char*>(&memory[offset]), size));
        }
    };
}

void runCpuDiagnostics()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        return;
    }

    SDL_Window* window = SDL_CreateWindow("CPU Diagnostics", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // holds the terminal output
    std::vector<std::string> consoleLines;
    consoleLines.push_back("");

    // flag to track if the text buffer was modified
    bool screenNeedsUpdate = true;

    auto printChar = [&](char c)
    {
        // something new arrived, queue a redraw
        screenNeedsUpdate = true;

        if (c == '\n' || c == '\r')
        {
            // skip empty redundant carriage returns
            if (c == '\r')
            {
                return;
            }
            consoleLines.push_back("");

            // keep a rolling buffer of 44 lines to fit a 640x480 window at 1x scale
            // this creates an automatic scrolling effect
            if (consoleLines.size() > 44)
            {
                consoleLines.erase(consoleLines.begin());
            }
        }
        else
        {
            consoleLines.back() += c;
        }
    };

    auto renderScreen = [&]()
    {
        SDL_SetRenderDrawColor(renderer, 0x10, 0x10, 0x18, 0xFF);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0x30, 0xFF, 0x30, 0xFF);

        int y = 10;
        for (const auto& line : consoleLines)
        {
            int x = 10;
            for (unsigned char c : line)
            {
                if (c > 127) c = '?';
                const unsigned char* glyph = font8x8_basic[c];

                for (int row = 0; row < 8; row++)
                {
                    for (int col = 0; col < 8; col++)
                    {
                        if ((glyph[row] >> col) & 0x01)
                        {
                            // render at 1x scale instead of 2x
                            SDL_Rect px{x + col, y + row, 1, 1};
                            SDL_RenderFillRect(renderer, &px);
                        }
                    }
                }
                // move forward 8 pixels per character
                x += 8;
            }
            // move down 10 pixels per line (8 for font + 2 padding)
            y += 10;
        }
        SDL_RenderPresent(renderer);
    };

    TestBus bus;
    CPU cpu(bus);

    std::vector<std::string> romPaths =
    {
        "./roms/cpu_tests/TST8080.COM",
        "./roms/cpu_tests/CPUTEST.COM",
        "./roms/cpu_tests/8080PRE.COM",
        "./roms/cpu_tests/8080EXM.COM"
    };

    bool userQuit = false;
    bool testFinished = false;

    bus.outHandlers[0] = [&](uint8_t)
    {
        testFinished = true;
    };

    bus.outHandlers[1] = [&](uint8_t)
    {
        if (cpu.C == 2)
        {
            printChar(static_cast<char>(cpu.E));
        }
        else if (cpu.C == 9)
        {
            uint16_t addr = cpu.DE;
            while (bus.readByte(addr) != '$')
            {
                printChar(static_cast<char>(bus.readByte(addr++)));
            }
        }
    };

    for (size_t i = 0; i < romPaths.size(); ++i)
    {
        if (userQuit)
        {
            break;
        }

        std::string fileName = std::filesystem::path(romPaths[i]).filename().string();

        // push a system message so we know what's running
        std::string header = std::string("--- RUNNING ") + fileName + " ---";
        for (char c : header) printChar(c);
        printChar('\n');

        for (int j = 0; j < 0x10000; j++)
        {
            bus.writeByte(j, 0x0);
        }

        if (!bus.loadROM(romPaths[i].c_str(), 0x0100))
        {
            std::string err = "skipped (rom not found)\n";
            for (char c : err) printChar(c);
            continue;
        }

        bus.writeByte(0x0000, 0xD3);
        bus.writeByte(0x0001, 0x00);

        bus.writeByte(0x0005, 0xD3);
        bus.writeByte(0x0006, 0x01);
        bus.writeByte(0x0007, 0xC9);

        cpu.PC = 0x0100;
        cpu.SP = 0xF000;

        testFinished = false;

        // main testing loop
        while (!testFinished && !userQuit)
        {
            // run a batch of instructions so we aren't completely
            // stalling the ui thread while chewing through huge tests
            for (int k = 0; k < 50000 && !testFinished; k++)
            {
                cpu.executeInstruction();
            }

            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    userQuit = true;
                }
                else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                {
                    userQuit = true;
                }
            }

            // only push pixels to the gpu if the text actually changed
            if (screenNeedsUpdate)
            {
                renderScreen();
                screenNeedsUpdate = false;
            }
        }

        // drop a couple of blank lines to visually separate this test from the next
        printChar('\n');
        printChar('\n');
    }

    // wait for them to dismiss the screen before tearing down
    while (!userQuit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
            {
                userQuit = true;
            }
        }
        renderScreen();
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}
