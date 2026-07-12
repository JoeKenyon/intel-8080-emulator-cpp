#include "../core/CPU.h"
#include "../ui/font8x8_basic.h"
#include "CpuDiagnostics.h"

#include <SDL2/SDL.h>
#include <algorithm>
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

    const int windowWidth  = 640;
    const int windowHeight = 380;
    const int lineHeight   = 10; // 8px glyph + 2px padding
    const int marginTop    = 10;
    const int marginLeft   = 10;

    // bottom row is reserved as a status bar, not part of the scrollable log
    const int linesPerScreen = (windowHeight - marginTop - lineHeight) / lineHeight;

    SDL_Window* window = SDL_CreateWindow("CPU Diagnostics", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // holds the full terminal output, nothing is ever deleted from this,
    // scrolling is just choosing which slice of it to draw
    std::vector<std::string> consoleLines;
    consoleLines.push_back("");

    bool screenNeedsUpdate = true;
    int  scrollOffset = 0;    // index of first visible line
    bool autoFollow    = true; // keep the view pinned to the newest line

    auto maxScrollOffset = [&]() -> int
    {
        return std::max(0, static_cast<int>(consoleLines.size()) - linesPerScreen);
    };

    auto scrollToBottom = [&]()
    {
        scrollOffset = maxScrollOffset();
    };

    auto printChar = [&](char c)
    {
        screenNeedsUpdate = true;

        if (c == '\n' || c == '\r')
        {
            if (c == '\r') return; // skip redundant carriage returns
            consoleLines.push_back("");
        }
        else
        {
            consoleLines.back() += c;
        }

        if (autoFollow) scrollToBottom();
    };

    auto drawText = [&](const std::string& text, int x, int y, SDL_Color color)
    {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        int penX = x;
        for (unsigned char c : text)
        {
            if (c > 127) c = '?';
            const unsigned char* glyph = font8x8_basic[c];

            for (int row = 0; row < 8; row++)
            {
                for (int col = 0; col < 8; col++)
                {
                    if ((glyph[row] >> col) & 0x01)
                    {
                        SDL_Rect px{penX + col, y + row, 1, 1};
                        SDL_RenderFillRect(renderer, &px);
                    }
                }
            }
            penX += 8;
        }
    };

    auto renderScreen = [&]()
    {
        SDL_SetRenderDrawColor(renderer, 0x10, 0x10, 0x18, 0xFF);
        SDL_RenderClear(renderer);

        int offset = std::min(scrollOffset, maxScrollOffset());
        int end    = std::min(static_cast<int>(consoleLines.size()), offset + linesPerScreen);

        int y = marginTop;
        for (int i = offset; i < end; i++)
        {
            drawText(consoleLines[i], marginLeft, y, SDL_Color{0x30, 0xFF, 0x30, 0xFF});
            y += lineHeight;
        }

        // status bar, always visible regardless of scroll position
        std::string status = autoFollow
            ? "-- LIVE (UP/PGUP TO SCROLL, ESC TO QUIT) --"
            : "-- PAUSED (DOWN/PGDN TO FOLLOW AGAIN, ESC TO QUIT) --";
        drawText(status, marginLeft, windowHeight - lineHeight, SDL_Color{0x70, 0x70, 0x70, 0xFF});

        SDL_RenderPresent(renderer);
    };

    auto handleScrollKey = [&](SDL_Keycode key) -> bool // returns true if it quit
    {
        switch (key)
        {
            case SDLK_ESCAPE:
                return true;
            case SDLK_UP:
                scrollOffset = std::max(0, scrollOffset - 1);
                autoFollow = false;
                screenNeedsUpdate = true;
                break;
            case SDLK_PAGEUP:
                scrollOffset = std::max(0, scrollOffset - linesPerScreen);
                autoFollow = false;
                screenNeedsUpdate = true;
                break;
            case SDLK_DOWN:
                scrollOffset = std::min(maxScrollOffset(), scrollOffset + 1);
                autoFollow = (scrollOffset >= maxScrollOffset());
                screenNeedsUpdate = true;
                break;
            case SDLK_PAGEDOWN:
                scrollOffset = std::min(maxScrollOffset(), scrollOffset + linesPerScreen);
                autoFollow = (scrollOffset >= maxScrollOffset());
                screenNeedsUpdate = true;
                break;
            default:
                break;
        }
        return false;
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

        while (!testFinished && !userQuit)
        {
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
                else if (event.type == SDL_KEYDOWN)
                {
                    if (handleScrollKey(event.key.keysym.sym)) userQuit = true;
                }
            }

            if (screenNeedsUpdate)
            {
                renderScreen();
                screenNeedsUpdate = false;
            }
        }

        printChar('\n');
        printChar('\n');
    }

    // stay open so the person can read/scroll through everything at their
    // own pace once the tests are done, instead of the window vanishing
    while (!userQuit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                userQuit = true;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if (handleScrollKey(event.key.keysym.sym)) userQuit = true;
            }
        }
        renderScreen();
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}