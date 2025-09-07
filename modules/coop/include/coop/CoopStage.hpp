#pragma once

#include <nsmb_nitro.hpp>
#include <nsmb/game/stage.hpp>

class Player;

namespace CoopStage {

extern u32 tempVar;
extern u8 isPlayerDead[2];
extern u8 forceAreaReload;
extern Player* flagpoleLinkedPlayer;

NTR_INLINE static bool isFlagpoleGrabbed()
{
	return flagpoleLinkedPlayer != nullptr;
}

NTR_INLINE static u16 getFgScreenID(u32 playerID)
{
	return *rcast<u16*>(&rcast<u8*>(Stage::stageLayout)[12 * playerID + 1196]);
}

// tempVar must be set to the target playerID before calling
void setCameraBound(StageLayout* self, s16 bound, u32 side);

void setCameraBoundAll(StageLayout* self, s16 bound, u32 side);
void matchPlayerCameraBounds(s32 playerID, s32 matchPlayerID);

// Use only if replacing setZoom calls in the original code, otherwise use setZoomAll
void setZoomAll_impl(fx32 zoom, u32 delay, u8 _playerID, u8 unk);

NTR_INLINE static void setZoomAll(fx32 zoom, u32 delay, u8 unk)
{
	// Force R2 to remain undefined as it is unused
	register int r2 asm("r2");
	return setZoomAll_impl(zoom, delay, r2, unk);
}

NTR_INLINE static bool isBossFight() { return *rcast<u32*>(0x020CA8C0) & 0x80000000; }
NTR_INLINE static bool hasLevelFinished() { return *rcast<u32*>(0x020CA8C0) & 1; }

s32 getAlivePlayerID();
bool areaHasRotator();
void blockForcedAreaReloads();
void drawStageIntroLuigiHead();

void defaultOnLevelLoad();
void defaultOnLayoutCreate();
void defaultOnLayoutUpdate();

} // namespace CoopStage
