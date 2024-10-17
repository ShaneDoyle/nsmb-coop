#pragma once

#include "nsmb_nitro.hpp"

class Player;

namespace PlayerSpectate {

u32 getTarget(u32 playerID);
void setTarget(u32 playerID, u32 targetPlayerID);
bool isSpectating(u32 playerID);
Player* getLocalTargetPlayer();
void reset();

}
