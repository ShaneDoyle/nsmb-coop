#pragma once

#include <nsmb_nitro.hpp>

namespace Coop {

constexpr u32 MaxPlayerCount = 2;

extern u8 isActive;

bool getLocalPlayerID();
bool isLuigiMode();
bool isMultiplayer();
bool isNotVS();

void syncSwitchScene(u32 r0, u32 r1);

} // namespace Coop
