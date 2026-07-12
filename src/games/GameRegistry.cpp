#include "GameRegistry.h"
#include "SpaceInvaders.h"
#include "LRescue.h"
#include "../tools/CpuDiagnostics.h"

std::vector<GameEntry> getMenuEntries()
{
    return
    {
        { "Space Invaders",  EntryKind::Game,       SpaceInvaders::buildConfig, nullptr },
        { "Lunar Rescue",    EntryKind::Game,       LunarRescue::buildConfig,   nullptr },
        { "CPU Diagnostics", EntryKind::Diagnostic, nullptr,                    runCpuDiagnostics },
    };
}
