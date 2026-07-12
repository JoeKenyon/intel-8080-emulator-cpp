#include "MenuScreen.h"
#include <iostream>

// Public domain 8x8 bitmap font (dhepper/font8x8, based on the classic IBM
// VGA font). Only included here, not in the header, so it's never defined
// twice across translation units.
#include "font8x8_basic.h"

namespace
{
    constexpr SDL_Color COLOR_BG        {0x10, 0x10, 0x18, 0xFF};
    constexpr SDL_Color COLOR_TITLE     {0xFF, 0xB0, 0x00, 0xFF}; // amber, arcade marquee style
    constexpr SDL_Color COLOR_ITEM      {0xC0, 0xC0, 0xC0, 0xFF};
    constexpr SDL_Color COLOR_SELECTED  {0x30, 0xFF, 0x30, 0xFF}; // classic phosphor green
    constexpr SDL_Color COLOR_FOOTER    {0x70, 0x70, 0x70, 0xFF};
}

MenuScreen::~MenuScreen()
{
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window)   SDL_DestroyWindow(m_window);
}

bool MenuScreen::initialize(const std::string& title) noexcept
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL Initialization Error: " << SDL_GetError() << "\n";
        return false;
    }

    m_window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 m_width, m_height, SDL_WINDOW_SHOWN);
    if (!m_window) return false;

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    return m_renderer != nullptr;
}

void MenuScreen::drawText(const std::string& text, int x, int y, int scale, SDL_Color color) noexcept
{
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);

    int penX = x;
    for (unsigned char c : text)
    {
        if (c > 127) c = '?';
        const unsigned char* glyph = font8x8_basic[c];

        for (int row = 0; row < 8; row++)
        {
            uint8_t bits = static_cast<uint8_t>(glyph[row]);
            for (int col = 0; col < 8; col++)
            {
                // change here: use col instead of (7 - col)
                bool on = (bits >> col) & 0x01;

                if (!on) continue;

                SDL_Rect px{ penX + col * scale, y + row * scale, scale, scale };
                SDL_RenderFillRect(m_renderer, &px);
            }
        }

        penX += 8 * scale;
    }
}

void MenuScreen::render(const std::vector<std::string>& items, int selected) noexcept
{
    SDL_SetRenderDrawColor(m_renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, COLOR_BG.a);
    SDL_RenderClear(m_renderer);

    drawText("TAITO 8080", 40, 40, 4, COLOR_TITLE);
    drawText("By Joe Kenyon", 40, 80, 2, COLOR_FOOTER);
    drawText("SELECT GAME", 40, 120, 3, COLOR_ITEM);

    int itemY = 160;
    for (size_t i = 0; i < items.size(); i++)
    {
        std::string upper = items[i];
        for (char& c : upper) c = static_cast<char>(::toupper(static_cast<unsigned char>(c)));

        bool isSelected = (static_cast<int>(i) == selected);
        std::string line = (isSelected ? "> " : "  ") + upper;
        drawText(line, 40, itemY, 2, isSelected ? COLOR_SELECTED : COLOR_ITEM);
        itemY += 30;
    }

    drawText("UP/DOWN SELECT", 40, m_height - 70, 1, COLOR_FOOTER);
    drawText("ENTER TO PLAY", 40, m_height - 55, 1, COLOR_FOOTER);
    drawText("ESC TO QUIT", 40, m_height - 40, 1, COLOR_FOOTER);

    SDL_RenderPresent(m_renderer);
}

int MenuScreen::run(const std::vector<std::string>& items) noexcept
{
    int selected = 0;
    bool open = true;
    int result = -1;

    while (open)
    {
        render(items, selected);

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                open = false;
                break;
            }

            if (event.type != SDL_KEYDOWN) continue;

            switch (event.key.keysym.sym)
            {
                case SDLK_UP:
                    selected = (selected == 0) ? static_cast<int>(items.size()) - 1 : selected - 1;
                    break;
                case SDLK_DOWN:
                    selected = (selected + 1) % static_cast<int>(items.size());
                    break;
                case SDLK_RETURN:
                case SDLK_SPACE:
                    result = selected;
                    open = false;
                    break;
                case SDLK_ESCAPE:
                    result = -1;
                    open = false;
                    break;
                default:
                    break;
            }
        }
    }

    return result;
}
