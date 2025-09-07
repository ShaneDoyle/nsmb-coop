#include <nsmb/game/game.hpp>
#include <nsmb/game/stage.hpp>
#include <nsmb/game/stage/actors/ov54/volcanoeruption.hpp>

extern "C" {
	void CoopSym_StageLayout_volcanoShake(StageLayout* self);
}

namespace CoopFixes::VolcanoEruption {

// Original Desync Issues:
// 1. Meteor spawns were triggered by the background animation function. This function can fail to update
//    for the local player under certain conditions (e.g., camera freeze, cutscenes, or death), causing
//    volcano eruptions to desynchronize between consoles.
// 2. The usual fix for actor targeting (replacing Game::getLocalPlayer with the closest player) doesn't apply here.
//    In this case, the code was already using the closest player, but this is still unreliable because the
//    volcano eruption actor's position is relative to the local camera. Each console sees the actor in a
//    different place, so only meteors spawned with the player pointer are synchronized; the actor itself is not.
//
// Fixes:
// - The eruption logic is now handled after StageLayout::onUpdate (see updateVolcanoBackground).
//   This ensures the eruption timer is updated consistently for all players.
// - Instead of targeting the closest player (which is unreliable due to local-only actor positions),
//   the code cycles through all players in order, so no one feels unfairly targeted and all consoles
//   spawn eruptions in sync.
//
// Additional changes:
// - Disabled the original background-triggered eruption and screen shake code with NOPs.
// - Patched the volcano eruption actor to allow out-of-range spawns, since range checks are not reliable
//   when actor positions are local-only.
// - Patched the actor's player targeting to use the round-robin method described above.

ncp_repl(0x020B6D2C, 0, "NOP") // Disable original volcano eruption trigger
ncp_repl(0x020B6D44, 0, "NOP") // Disable original volcano eruption trigger
ncp_repl(0x020B6D6C, 0, "NOP") // Disable original screen shake

constexpr u16 timerInterval = 60 * 8;
u16 timer = 60 * 8 - 1;
u8 targetPlayer = 0;

struct bgScrollData
{
    fx32 field_0;
    fx32 bottomScrollF32;
    fx32 cameraOffset;
    fx32 topScrollF32;
};

ncp_thumb void reset()
{
	timer = timerInterval - 1;
	targetPlayer = 0;
}

ncp_thumb void updateBackground()
{
	u32 playerID = Game::localPlayerID;

	timer++;
	if (timer != timerInterval)
		return;
	timer = 0;

	u8 viewID = Game::getPlayer(playerID)->viewID;

	bgScrollData* gStageHorizontalScrollData = rcast<bgScrollData*>(0x020CAF18);
	bgScrollData* gStageVerticalScrollData = rcast<bgScrollData*>(0x020CAF38);

	fx32 horizScroll = gStageHorizontalScrollData[playerID].topScrollF32 & 0xFF000;
	fx32 vertScroll  = gStageVerticalScrollData[playerID].topScrollF32 & 0x1FF000;
	fx32 horizOffset = gStageHorizontalScrollData[playerID].cameraOffset;
	fx32 vertOffset  = gStageVerticalScrollData[playerID].cameraOffset;

	// Compute eruption position
	Vec3 eruptionPos;
	eruptionPos.x = (0x2A000 - horizScroll) + horizOffset;
	eruptionPos.y = -(0x150000 - vertScroll + vertOffset);
	eruptionPos.z = 0;

	// First eruption
	::VolcanoEruption::erupt(eruptionPos, viewID);

	// Second eruption, shifted right
	eruptionPos.x += 0x100000;
	::VolcanoEruption::erupt(eruptionPos, viewID);

	targetPlayer++;
	if (targetPlayer == Game::getPlayerCount())
		targetPlayer = 0;

	CoopSym_StageLayout_volcanoShake(Stage::stageLayout);
}

// We cannot use the closest player for targeting, since the actor's position is local-only.
// Instead, cycle through all players in order to distribute eruptions fairly.
ncp_call(0x021633DC, 54)
Player* fixGetClosestPlayerOnCreate()
{
	return Game::getPlayer(targetPlayer);
}

// Allow volcano eruptions to spawn even if the player is out of range,
// since range checks are unreliable with local-only actor positions.
ncp_repl(0x02162F2C, 54, "MOV R0, #0")

} // namespace CoopFixes::VolcanoEruption
