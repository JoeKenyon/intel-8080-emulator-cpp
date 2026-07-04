#include <SDL2/SDL.h>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "emulator.h"

// ───────────────────────────────
// SCREEN
// ───────────────────────────────
static constexpr int WIDTH  = 224;
static constexpr int HEIGHT = 256;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* texture = nullptr;

uint32_t framebuffer[WIDTH * HEIGHT];

// ───────────────────────────────
// VIDEO RENDER
// ───────────────────────────────
void drawScreen()
{
    std::memset(framebuffer, 0, sizeof(framebuffer));
    uint8_t* video = &memory[0x2400];

    for (int i = 0; i < WIDTH; i++) // i goes 0 to 223 (X axis columns)
    {
        for (int j = 0; j < 32; j++) // j goes 0 to 31 (8-pixel vertical chunks)
        {
            uint8_t b = video[i * 32 + j];
            for (int bit = 0; bit < 8; bit++)
            {
                int px = i;                   // X is simply the column
                int py = 255 - (j * 8 + bit); // Y starts at bottom (255) and goes up

                bool on = b & (1 << bit);
                framebuffer[py * WIDTH + px] = on ? 0xFFFFFFFF : 0xFF000000;
            }
        }
    }

    SDL_UpdateTexture(texture, nullptr, framebuffer, WIDTH * sizeof(uint32_t));
}

// ───────────────────────────────
// INTERRUPTS
// ───────────────────────────────
void push16(uint16_t v)
{
    memory[--SP] = (v >> 8) & 0xFF;
    memory[--SP] = v & 0xFF;
}

void triggerInterrupt(uint8_t vector)
{
    push16(PC);
    PC = vector;
}

// ───────────────────────────────
// INPUT
// ───────────────────────────────
uint8_t port1 = 0; // P1 controls
uint8_t port2 = 0; // P2 controls (optional for now)

void handleInput()
{
    const Uint8* keys = SDL_GetKeyboardState(nullptr);

    port1 = 0;

    // Space Invaders Port 1 Bitmap:
    // Bit 0: Coin
    // Bit 1: P2 Start
    // Bit 2: P1 Start
    // Bit 4: P1 Shoot
    // Bit 5: P1 Left
    // Bit 6: P1 Right

    if (keys[SDL_SCANCODE_C])     port1 |= (1 << 0); // Insert Coin
    if (keys[SDL_SCANCODE_1])     port1 |= (1 << 2); // Player 1 Start
    if (keys[SDL_SCANCODE_SPACE]) port1 |= (1 << 4); // Shoot
    if (keys[SDL_SCANCODE_LEFT])  port1 |= (1 << 5); // Left
    if (keys[SDL_SCANCODE_RIGHT]) port1 |= (1 << 6); // Right
}

// ───────────────────────────────
// MAIN
// ───────────────────────────────
int main()
{
    // ─── LOAD ROMS ───
    if (!loadRom("roms/invaders.h", 0x0000) ||
        !loadRom("roms/invaders.g", 0x0800) ||
        !loadRom("roms/invaders.f", 0x1000) ||
        !loadRom("roms/invaders.e", 0x1800))
    {
        std::cerr << "ROM load failed\n";
        return 1;
    }

    // ─── CPU INIT ───
    PC = 0x0000;
    SP = 0xF000;

    // ─── SDL INIT ───
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow(
        "8080 Space Invaders",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH * 3,
        HEIGHT * 3,
        SDL_WINDOW_SHOWN);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888, // IMPORTANT FIX
        SDL_TEXTUREACCESS_STREAMING,
        WIDTH,
        HEIGHT);

    bool running = true;

    bool irqFlip = false;

    // timing
    const double FPS = 60.0;
    const int CYCLES_PER_FRAME = 2000000 / FPS;

    // ─── MAIN LOOP ───
    // ─── MAIN LOOP ───
    const int INSTRUCTIONS_PER_FRAME = 33333 / 5; // roughly 6666 instructions per frame

    while (running)
    {
        uint64_t frameStart = SDL_GetTicks64(); // Track when the frame started

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT) running = false;
        }

        handleInput();

        // ─── CPU EXECUTION (FIRST HALF OF SCREEN) ───
        int executed = 0;
        while (executed < INSTRUCTIONS_PER_FRAME / 2)
        {
            executed += step(); // Assuming this returns 1 per instruction
        }

        // The arcade hardware fires RST 1 when the electron beam hits the middle of the screen
        if (interrupts.enabled) {
            interrupts.enabled = false;
            triggerInterrupt(0x0008);
        }

        // ─── CPU EXECUTION (SECOND HALF OF SCREEN) ───
        while (executed < INSTRUCTIONS_PER_FRAME)
        {
            executed += step();
        }

        // The arcade hardware fires RST 2 at the bottom of the screen (VBLANK)
        if (interrupts.enabled) {
            interrupts.enabled = false;
            triggerInterrupt(0x0010);
        }

        // ─── RENDER ───
        drawScreen();
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        // ─── FRAMERATE LOCK (60 FPS) ───
        uint64_t frameTime = SDL_GetTicks64() - frameStart;
        if (frameTime < 16)
        {
            SDL_Delay(16 - frameTime); // Sleep for the remaining milliseconds
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
