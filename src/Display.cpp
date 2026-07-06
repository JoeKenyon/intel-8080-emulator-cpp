#include "Display.h"
#include <iostream>

Display::~Display()
{
    if (m_texture)  SDL_DestroyTexture(m_texture);
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window)   SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool Display::initialize(const char* title, int scale) noexcept
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL Initialization Error: " << SDL_GetError() << "\n";
        return false;
    }

    m_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                NATIVE_WIDTH * scale, NATIVE_HEIGHT * scale, SDL_WINDOW_SHOWN);
    if (!m_window) return false;

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) return false;

    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                  NATIVE_WIDTH, NATIVE_HEIGHT);
    return m_texture != nullptr;
}

void Display::handleInput(std::array<uint8_t, 256>& ports) noexcept
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            m_isOpen = false;
            return;
        }

        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
        {
            bool isDown = (event.type == SDL_KEYDOWN);

            switch (event.key.keysym.sym)
            {
                // Port 1 Mapping Configurations
                case SDLK_c: // Insert Coin
                    if (isDown) ports[1] |= 0b0000'0001; else ports[1] &= ~0b0000'0001; break;
                case SDLK_RETURN: // Player 1 Start
                    if (isDown) ports[1] |= 0b0000'0100; else ports[1] &= ~0b0000'0100; break;
                case SDLK_SPACE: // Player 1 Shoot
                    if (isDown) ports[1] |= 0b0001'0000; else ports[1] &= ~0b0001'0000; break;
                case SDLK_LEFT: // Player 1 Move Left
                    if (isDown) ports[1] |= 0b0010'0000; else ports[1] &= ~0b0010'0000; break;
                case SDLK_RIGHT: // Player 1 Move Right
                    if (isDown) ports[1] |= 0b0100'0000; else ports[1] &= ~0b0100'0000; break;

                // Escape safety valve
                case SDLK_ESCAPE:
                    m_isOpen = false;
                    break;
            }
        }
    }

    ports[1] |= 0x08;  // Port 1, Bit 3 always high
    ports[2] &= ~0x04; // Clear Port 2, Bit 2, turn off the TILT sensor
    ports[2] |= 0x40;  // Port 2, Bit 6 always high (Coin-door chassis wiring loop)
}

void Display::render(const std::array<uint8_t, 0x10000>& memory) noexcept
{

    // VRAM starts at 0x2400. Each byte represents 8 vertical pixels.
    uint16_t vramStart = 0x2400;

    // Process original VRAM byte columns
    for (int i = 0; i < NATIVE_WIDTH; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            uint8_t b = memory[(i * 32 + j) + vramStart];
            for (int bit = 0; bit < 8; bit++)
            {
                int px = i;
                int py = 255 - (j * 8 + bit);

                bool on = b & (1 << bit);
                m_screenBuffer[py * NATIVE_WIDTH + px] = on ? 0xFFFFFFFF : 0xFF000000;
            }
        }
    }

    SDL_UpdateTexture(m_texture, nullptr, m_screenBuffer.data(), NATIVE_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}
