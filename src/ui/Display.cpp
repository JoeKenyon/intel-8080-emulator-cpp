#include "Display.h"
#include <iostream>

Display::~Display()
{
    if (m_texture)  SDL_DestroyTexture(m_texture);
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window)   SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool Display::initialize(const MachineConfig& config) noexcept
{
    m_width     = config.screenWidth;
    m_height    = config.screenHeight;
    m_vramStart = config.vramStart;
    m_screenBuffer.assign(static_cast<size_t>(m_width) * m_height, 0xFF000000);

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL Initialization Error: " << SDL_GetError() << "\n";
        return false;
    }

    m_window = SDL_CreateWindow(config.name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 m_width * config.windowScale, m_height * config.windowScale, SDL_WINDOW_SHOWN);
    if (!m_window) return false;

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) return false;

    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                   m_width, m_height);
    return m_texture != nullptr;
}

void Display::handleInput(std::array<uint8_t, 256>& ports, const std::vector<KeyBinding>& bindings) noexcept
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            m_isOpen = false;
            return;
        }

        if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP)
        {
            continue;
        }

        // SDL fires repeated SDL_KEYDOWN events while a key is held; without this
        // guard held keys would spam OR onto the port every frame (harmless here
        // since it's already 1, but it's the kind of thing that bites you later).
        if (event.key.repeat)
        {
            continue;
        }

        bool isDown = (event.type == SDL_KEYDOWN);

        if (event.key.keysym.sym == SDLK_ESCAPE)
        {
            if (isDown) m_isOpen = false;
            
            continue;
        }

        for (const auto& binding : bindings)
        {
            if (binding.key != event.key.keysym.sym) continue;

            if (isDown) ports[binding.port] |= binding.mask;
            else        ports[binding.port] &= static_cast<uint8_t>(~binding.mask);
        }
    }
}

void Display::render(const std::array<uint8_t, 0x10000>& memory) noexcept
{
    // 1bpp framebuffer: each byte is 8 vertical pixels, columns stored consecutively.
    const int bytesPerColumn = m_height / 8;

    for (int col = 0; col < m_width; col++)
    {
        for (int byteIdx = 0; byteIdx < bytesPerColumn; byteIdx++)
        {
            uint8_t b = memory[m_vramStart + col * bytesPerColumn + byteIdx];
            for (int bit = 0; bit < 8; bit++)
            {
                int px = col;
                int py = (m_height - 1) - (byteIdx * 8 + bit);

                bool on = b & (1 << bit);
                m_screenBuffer[py * m_width + px] = on ? 0xFFFFFFFF : 0xFF000000;
            }
        }
    }

    SDL_UpdateTexture(m_texture, nullptr, m_screenBuffer.data(), m_width * sizeof(uint32_t));
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}