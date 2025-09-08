#include "coop/CoopStage.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/game/sound/sound.hpp>
#include <nsmb/game/player.hpp>
#include <nsmb/game/ui.hpp>
#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/misc.hpp>
#include <nsmb/game/stage/player/player.hpp>
#include <nsmb/game/stage/player/door.hpp>
#include <nsmb/game/stage/layout/data/entrance.hpp>
#include <nsmb/core/graphics/particle.hpp>
#include <nsmb/core/system/input.hpp>
#include <nsmb/core/system/function.hpp>
#include <nsmb/core/system/misc.hpp>
#include <nsmb/core/graphics/fader.hpp>
#include <nsmb/core/entity/scene.hpp>
#include <nsmb/core/wifi/wifi.hpp>
#include <nsmb/core/filesystem.hpp>

#include "fid.hpp"
#include "coop/Coop.hpp"
#include "coop/CoopStage.hpp"
#include "coop/CoopCamera.hpp"
#include "coop/fixes/VolcanoEruption.hpp"
#include "coop/CoopPlayer.hpp"
#include "coop/DesyncGuard.hpp"
#include "nwav/nwav.hpp"

namespace CoopFixes::Stage {

extern "C" {
	void CoopSym_StageLayout_looperScrollBack(void* stageLayout, s32 playerID);
	void CoopSym_SetupGraphicsForBottomScreenInStage(int);
}

// ======================================= PAUSE =======================================

#ifdef COOP_FIX_PAUSE_MENU

ncp_asmfunc void fixPauseMenuDesyncs_ASM()
{asm(R"(
// Fix desyncs on pause menu
ncp_call(0x020A20EC, 0)
ncp_call(0x020A2280, 0)
ncp_call(0x020A23B4, 0)
	ADD     R1, R5, #0x6000
	LDRB    R1, [R1,#0x428] // R1 = pause menu owner
	MOV     R1, R1,LSL#1
	LDRH    R1, [R0,R1] // R1 = Input::consoleKeysRepeated[R1]
	BX      LR

ncp_call(0x020A21A4, 0)
ncp_call(0x020A22D8, 0)
	LDR     R0, =0x6428
	LDRB    R0, [R5,R0] // R0 = pause menu owner
	BX      LR

// Disable options on pause menu
ncp_jump(0x020A2230, 0)
	BL      _ZN4Game14getPlayerCountEv
	CMP     R0, #1
	BLEQ    0x020C1F14
	B       0x020A24D0
)");}

#endif // COOP_FIX_PAUSE_MENU

// ======================================= LOOPER =======================================

#ifdef COOP_FIX_LOOPER

ncp_repl(0x020BBD64, 0, "NOP") // Moved to StageLayout_onCreateHook

void customLooperScrollBack(void* stageLayout)
{
	u8* looperApplyLoop = rcast<u8*>(0x020CACDC);

	for (u32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		if (looperApplyLoop[playerID])
			CoopSym_StageLayout_looperScrollBack(stageLayout, playerID);
	}
}

ncp_asmfunc void customLooperScrollBackCall_ASM()
{asm(R"(
ncp_over(0x020AE8F0, 0)
	MOV     R0, R6
	BL      _ZN9CoopFixes5Stage22customLooperScrollBackEPv
	B       0x020AE90C
ncp_endover()
)");}

#endif // COOP_FIX_LOOPER

// ======================================= MISC =======================================

// Fix camera set bound desync
ncp_set_jump(0x020ACF50, 0, CoopStage::setCameraBoundAll)

ncp_asmfunc void fixBG1CNTForOtherPlayer_ASM()
{asm(R"(
// Do not set the BG1 CNT with the other player's data, we don't have it!!! (Fix 2-4 background)
ncp_jump(0x020BA3AC, 0)
    LDR     R2, =_ZN4Game13localPlayerIDE
    LDR     R2, [R2]
    CMP     R2, R4
    BNE     0x020BA488 // Do not apply the BG changes
    LDRH    R2, [R0] // Keep replaced instruction
    B       0x020BA3B0
)");}

// Prevent reloading resources on preCreate during multiplayer wait (Fixes fading)
static bool StageScene_preCreate(Scene* self)
{
	if (rcast<u32*>(self)[0x640C / 4])
		return true;
	return self->Scene::preCreate();
}

ncp_over(0x020C6E68, 0) const auto StageScene_preCreate_vtbl = StageScene_preCreate;

// No idea what these do
// ncp_repl(0x0209B254, 0, "MOV R0, #1")
// ncp_repl(0x0209BD2C, 0, "MOV R0, #1")
// ncp_repl(0x020D06E0, 10, "MOV R0, #1")

// Fix bottom screen transition flag desync

ncp_thumb void copyTransitionFlagsIfAreaChange(s32 playerID)
{
	u32& areaNum = *rcast<u32*>(0x02085A94);
	if (Entrance::targetAreaID != areaNum || CoopStage::forceAreaReload)
	{
		s32 otherID = playerID ^ 1;
		Entrance::transitionFlags[otherID] = Entrance::transitionFlags[playerID];
	}
}

ncp_call(0x0215E7C4, 54)
void copyTransitionFlagsOnLoad(int arg)
{
	// Set the flags for the other player too
	Entrance::transitionFlags[Game::localPlayerID ^ 1] = Entrance::transitionFlags[Game::localPlayerID];

	CoopSym_SetupGraphicsForBottomScreenInStage(arg); // Keep replaced instruction
}

ncp_asmfunc void copyTransitionFlagsOnAreaChange_ASM()
{asm(R"(
ncp_jump(0x0201EBE0)
	LDR     R0, [SP,#0] // playerID
	BL      _ZN9CoopFixes5Stage31copyTransitionFlagsIfAreaChangeEl
	ADD     SP, SP, #4
	B       0x0201EBE4
)");}

ncp_asmfunc void disableBaphs_ASM()
{asm(R"(
// Disable baphs if player count is bigger than 1 (prevents desyncs)
ncp_jump(0x02012584)
	BL      _ZN4Game14getPlayerCountEv
	CMP     R0, #1
	BGT     0x0201258C
	LDR     R0, =0x02088B94
	B       0x02012588
)");}

ncp_repl(0x020FBF60, 10, "BX LR") // Fix end of level for player that "lost the race"

ncp_set_call(0x021624C8, 54, Coop::getLocalPlayerID) // Midway point draws from local player
ncp_set_call(0x02162110, 54, Coop::getLocalPlayerID) // Midway point plays sound at local player position

ncp_repl(0x0215ED54, 54, "NOP") // Disable mega mushroom destruction counter

#ifdef COOP_FIX_STAGE_INTRO

//ncp_set_call(0x02152944, 54, Coop::isLuigiMode) // Allow Luigi lives on stage intro scene
//ncp_set_call(0x0215293C, 54, Coop::isLuigiMode) // Allow Luigi head on stage intro scene

ncp_call(0x02152A4C, 54)
ncp_thumb s32 StageIntroScene_onRender_hook()
{
	if (Game::getPlayerCount() == 1)
		return 1;

	CoopStage::drawStageIntroLuigiHead();
	return 1;
}

ncp_set_call(0x02152874, 54, Coop::syncSwitchScene)

#endif

ncp_repl(0x020FBD70, 10, "NOP") // Disables "Lose" music. (End Flag & Boss)

ncp_asmfunc void spawnEnemiesFromMegaGroundPound_ASM()
{asm(R"(
// Store Player* for SpawnEnemiesFromMegaGroundPound
ncp_jump(0x021121EC, 10)
	LDR     R0, =_ZN9CoopStage7tempVarE
	STR     R5, [R0]
	BL      0x0209E038 // SpawnEnemiesFromMegaGroundPound
	B       0x021121F0

// SpawnEnemiesFromMegaGroundPound
ncp_jump(0x0209E0D0, 0)
	LDR     R0, =_ZN9CoopStage7tempVarE
	LDR     R0, [R0]
	B       0x0209E0D4

// Pass the playerID to the drop controller settings
ncp_jump(0x0209E108, 0)
	ORR     R1, R0, #0x10000000
	LDR     R0, =_ZN9CoopStage7tempVarE
	LDR     R0, [R0]
	ADD     R0, R0, #0x100
	LDRSB   R0, [R0,#0x1E] // playerID
	MOV     R0, R0,LSL#16
	ORR     R1, R1, R0
	B       0x0209E10C

ncp_jump(0x02157414, 54)
	LDRB    R0, [R5,#10] // (settings >> 16) & 0xFF
	BL      _ZN4Game9getPlayerEl
	B       0x02157418
)");}

// TODO: Fix Mega Mushroom destruction counter

// Do not know what this does yet
/*asm(R"(
ncp_jump(0x020BE184, 0)
	LDR     R0, =_ZN4Game11playerCountE
	B       0x020BE188

ncp_over(0x020BE18C, 0)
	CMP     R0, #1
	MOVGT   R0, #1
	MOVLE   R0, #0
ncp_endover()
)");*/

// Fix stage zoom on view edges
ncp_asmfunc void fixStageZoomOnViewEdges_ASM()
{asm(R"(
ncp_jump(0x020BA1C4, 0)
	MOV     R5, R3
	LDR     R3, =_ZN9CoopStage7tempVarE
	STR     R5, [R3]
	B       0x020BA1C8
)");}

ncp_repl(0x020B8D20, 0, ".int _ZN9CoopStage7tempVarE")

// Disable entrance camera X and Y
// TODO: Find a fix for this that doesn't involve disabling it

ncp_asmfunc void disableEntranceCameraXY_ASM()
{asm(R"(
ncp_over(0x020BC67C, 0)
	LDR     R0, .over_0x020BC67C_0_vars
	MOV     R2, #0
	STRB    R2, [R0,R1]
	BX      LR
.over_0x020BC67C_0_vars:
	.word	0x020CACC0
ncp_endover()
)");}

// Do not clear event timers on view change, only area reload

ncp_asmfunc void doNotClearEventTimersOnViewChange_ASM()
{asm(R"(
ncp_jump(0x02118860, 10)
	BL      _ZN4Game14getPlayerCountEv
	CMP     R0, #1
	BLEQ    0x0201DD5C // clearEventTimers
	B       0x02118E00
)");}

// Fix the level rotator

/*
ncp_jump(0x020B0A38, 0)
void StageLayout_fixRotatorStep2Check(void* self)
{
	fx32* manualCameraScroll = rcast<fx32*>(self) + (0x564/4);
	u8& rotatorStep = *rcast<u8*>(0x020CAC74);

	manualCameraScroll[0] = 0;
	manualCameraScroll[1] = 0;

	rotatorStep = 2;
}
*/

ncp_over(0x020B0A38, 0) /* max over: 0x94 bytes, current: 0x20 bytes */
ncp_asmfunc void StageLayout_fixRotatorStep2Check_ASM()
{asm(R"(
	mov	r3, #0
	mov	r2, #2
	str	r3, [r0, #1380]
	str	r3, [r0, #1384]
	ldr	r3, .rotfix_L7
	strb	r2, [r3, #116]
	bx	lr
.rotfix_L7:
	.word	34384896
)");}

// Not fully sure of the effect of this function on the level.
// It appears to displace the actors, but it is causing desyncs.
// TODO: Investigate actor displacement for rotator.
ncp_repl(0x020AE7EC, 0, "NOP")

// During the level rotation, Stage::cameraX[1] desyncs on console 0.
// Since it only happens during the rotation, it seems to be safe.
// The patch commented below disables the code that causes the problem,
// ncp_repl(0x020B0B68, 0, "NOP")

// Fix forced cutscene camera scroll

u32 forcedHCameraScroll[2];

ncp_asmfunc void fixForcedHCameraScroll_ASM()
{asm(R"(
// Initialize for both players
ncp_jump(0x020BBEAC, 0)
	LDR     R1, =_ZN9CoopFixes5Stage19forcedHCameraScrollE
	STR     R0, [R1]
	STR     R0, [R1,#4]
	B       0x020BBEB0

// Read the values per player
ncp_jump(0x020B9A0C, 0)
	LDR     R3, =_ZN9CoopFixes5Stage19forcedHCameraScrollE
	LDR     R4, [R3,R6,LSL#2]
	B       0x020B9A10

ncp_jump(0x020B9B04, 0)
	LDR     R0, =_ZN9CoopFixes5Stage19forcedHCameraScrollE
	LDR     R2, [R0,R4,LSL#2]
	B       0x020B9B08

// Write the value for both players
ncp_over(0x020B9B10, 0)
	STR     R1, [R0,R4,LSL#2]
ncp_endover()

ncp_jump(0x0213A8B0, 13)
	STR     R2, [R1]
	STR     R2, [R1,#4]
	B       0x0213A8B4

ncp_call(0x0213AAA0, 13)
ncp_call(0x0214599C, 40)
	STR     R3, [R1]
	STR     R3, [R1,#4]
	BX      LR
)");}

ncp_repl(0x0213AD18, 13, ".int _ZN9CoopFixes5Stage19forcedHCameraScrollE")
ncp_repl(0x02145CE8, 40, ".int _ZN9CoopFixes5Stage19forcedHCameraScrollE")

#ifdef COOP_FIX_PIPES_BG

// Fix the pipes background

// There is a check for background ID 29 (pipes) that sets a camera offset.
// The issue is that screenFG data is only available for the local player, but the
// original code was accessing it with any playerID, causing a desync.
// Nintendo was aware of this issue so they just disabled this offset in VS mode,
// but nsmb-coop doesn't use VS mode.

ncp_asmfunc void StageLayout_fixPipesBackground_ASM()
{asm(R"(
ncp_jump(0x020B286C, 0)
	// R7 = playerID
	// R3 safe to use
    LDR     R3, =_ZN4Game13localPlayerIDE
    LDR     R3, [R3]
	MLA     R1, R3, R1, R10 // Replace R7 with R3 (use localPlayerID instead of playerID)
	B       0x020B2870
)");}

#endif

ncp_repl(0x020C0660, 0, "B 0x020C067C") // Do not skip local player check for empty inventory SFX

} // namespace CoopFixes::Stage
