#pragma once
#include "../core/MachineConfig.h"
#include <functional>
#include <string>
#include <variant>
#include <vector>

// One entry per menu item. Adding a playable game or a diagnostic tool to
// the app is just adding a line to getMenuEntries() below, nothing else
// needs to change.
//
// The action is a variant rather than "kind + two nullable fields" so an
// entry can't be constructed half-wired (e.g. a Diagnostic with no
// runAction, or a Game with buildConfig left null) - it just won't compile.
struct GameEntry
{
    std::string name;

    using BuildConfig = std::function<MachineConfig()>; // boots the Emulator
    using RunAction    = std::function<void()>;          // runs standalone instead

    std::variant<BuildConfig, RunAction> action;
};

std::vector<GameEntry> getMenuEntries();