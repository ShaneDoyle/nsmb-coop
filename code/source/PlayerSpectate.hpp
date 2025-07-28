#pragma once

#include <nsmb_nitro.hpp>

class Player;

namespace PlayerSpectate {

u32 getTarget(u32 playerID);
void setTarget(u32 playerID, u32 targetPlayerID);
void setLerping(u32 playerID, bool lerping);
void enableSharedCamera();
bool isSpectating(u32 playerID);
Player* getTargetPlayer(u32 playerID);
Player* getLocalTargetPlayer();
void clearSpectators();
void onStageLayoutCreate();
void onStageLayoutUpdate();
void onViewTransit(u8 transitPlayerID);

}
