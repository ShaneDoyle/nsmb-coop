#pragma once

#include <nsmb_nitro.hpp>

class Player;

extern u8 Stage_isPlayerDead[2];
extern Player* Stage_flagpoleLinkedPlayer;

static inline bool Stage_isBossFight() { return *rcast<u32*>(0x020CA8C0) & 0x80000000; }
static inline bool Stage_hasLevelFinished() { return *rcast<u32*>(0x020CA8C0) & 1; }
bool Stage_areaHasRotator();
