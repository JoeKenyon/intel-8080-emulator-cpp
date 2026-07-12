#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>

// Simple arcade-cabinet style "insert coin" screen: lists entries, lets the
// user move a cursor with up/down and confirm with enter. Runs before any
// game-specific window exists, so it owns its own small SDL window and tears
// it down again once a choice is made (or the user quits).
class MenuScreen
{
public:
    ~MenuScreen();

    bool initialize(const std::string& title = "TAITO 8080") noexcept;

    // Returns the chosen index into `items`, or -1 if the user quit.
    int run(const std::vector<std::string>& items) noexcept;

private:
    void drawText(const std::string& text, int x, int y, int scale, SDL_Color color) noexcept;
    void render(const std::vector<std::string>& items, int selected) noexcept;

    SDL_Window*   m_window{nullptr};
    SDL_Renderer* m_renderer{nullptr};

    int m_width{480};
    int m_height{480};
};
