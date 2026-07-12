#pragma once
#include "../core/MachineConfig.h"
#include <functional>
#include <string>
#include <vector>

enum class EntryKind
{
    Game,       // boots the Emulator with a MachineConfig
    Diagnostic  // runs some standalone action instead (e.g. CPU test suite)
};

// One entry per menu item. Adding a playable game or a diagnostic tool to
// the app is just adding a line to getMenuEntries() below, nothing else
// needs to change.
struct GameEntry
{
    std::string name;
    EntryKind kind = EntryKind::Game;

    std::function<MachineConfig()> buildConfig;  // used when kind == Game
    std::function<void()>          runAction;    // used when kind == Diagnostic
};

std::vector<GameEntry> getMenuEntries();
