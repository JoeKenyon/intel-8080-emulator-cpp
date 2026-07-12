#include "./core/Emulator.h"
#include "./games/GameRegistry.h"
#include "./ui/MenuScreen.h"
#include <variant>
#include <vector>
#include <string>

int main()
{
    const auto entries = getMenuEntries();

    std::vector<std::string> labels;
    labels.reserve(entries.size());
    for (const auto& entry : entries)
    {
        labels.push_back(entry.name);
    }

    // wrap the main flow in a loop so it keeps returning to the menu
    while (true)
    {
        int choice = -1;
        {
            // scoped so the menu window and its sdl context are torn down
            // before a game or diagnostic gets to open its own window
            MenuScreen menu;
            if (!menu.initialize())
            {
                return 1;
            }
            choice = menu.run(labels);
        }

        // user quit from the menu screen directly
        if (choice < 0)
        {
            break;
        }

        const GameEntry& chosen = entries[choice];

        std::visit([](const auto& action)
        {
            using ActionType = std::decay_t<decltype(action)>;

            if constexpr (std::is_same_v<ActionType, GameEntry::RunAction>)
            {
                action();
            }
            else
            {
                Emulator emulator(action());

                if (emulator.boot())
                {
                    // the emulator will block here until it naturally quits
                    // once it returns, the loop restarts and shows the menu again
                    emulator.run();
                }
            }
        }, chosen.action);
    }

    return 0;
}