#include "GameRegistry.h"
#include "SpaceInvaders.h"
#include "LRescue.h"
#include "../tools/CpuDiagnostics.h"

std::vector<GameEntry> getMenuEntries()
{
    return
    {
        { "Space Invaders",  GameEntry::BuildConfig{ SpaceInvaders::buildConfig } },
        { "Lunar Rescue",    GameEntry::BuildConfig{ LunarRescue::buildConfig   } },
        { "CPU Diagnostics", GameEntry::RunAction{ runCpuDiagnostics }          },
    };
}