#include "coop/CoopStage.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/game/sound/sound.hpp>
#include <nsmb/game/player.hpp>
#include <nsmb/game/ui.hpp>
#include <nsmb/game/stage/entity.hpp>
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
#include "coop/CoopPlayer.hpp"
#include "coop/CoopCamera.hpp"
#include "coop/DesyncGuard.hpp"
#include "coop/actors/CoopFlagActor.hpp"
#include "coop/fixes/Liquid.hpp"
#include "coop/fixes/VolcanoEruption.hpp"
#include "nwav/nwav.hpp"
#include "coop/util/ThumbBarrier.hpp"

// Purpose of this namespace:
// Provide common helpers
// Separate VS logic from Story logic

extern "C" {
	void CoopSym_SpawnGrowingEntranceVine(Vec3*);
	void CoopSym_StageLayout_setupView(StageLayout* self, Vec3* focusPos, s32 playerID, u32 isFromStageOnCreate);
	void CoopSym_DrawBottomScreenLives();
	void CoopSym_loadCastleModel();
	extern fx32 CoopSym_Stage_zoom[2];
}

namespace CoopStage {

constexpr u32 spectateTextFileID = "coop/A_text_spectate_ncg.bin"fid;

u32 tempVar;
u8 isPlayerDead[2];
u8 doorFromAreaChange;
u8 forceAreaReload;
u8 forcedAreaReloadsBlocked;
Player* flagpoleLinkedPlayer; // Player in the pole responsible for triggering events
u8 playerInFlagpoleSentFlying[Coop::MaxPlayerCount];

s32 getAlivePlayerID()
{
	if (isPlayerDead[0])
		return 1;
	if (isPlayerDead[1])
		return 0;
	return 2; // both
}

ncp_thumb bool areaHasRotator()
{
	u32 tilesetCount = Stage::getBlockElementCount(scast<u32>(StageBlockID::Tileset));
	for (u32 i = 0; i < tilesetCount; i++)
	{
		if (Stage::stageBlocks.tileset[i].screenID == 0xFF00)
			return true;
	}
	return false;
}

void blockForcedAreaReloads()
{
	forcedAreaReloadsBlocked = true;
}

ncp_asmfunc void setCameraBound(StageLayout* self, s16 bound, u32 side)
{asm(R"(
	LDR     R12, =_ZN9CoopStage7tempVarE
	B       0x020ACF54
)");}

ncp_thumb void setCameraBoundAll(StageLayout* self, s16 bound, u32 side)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		tempVar = playerID;
		setCameraBound(self, bound, side);
	}
}

ncp_thumb void matchPlayerCameraBounds(s32 playerID, s32 matchPlayerID)
{
	s16 boundX = Stage::cameraX[matchPlayerID] >> FX32_SHIFT;

	tempVar = playerID;
	setCameraBound(Stage::stageLayout, boundX, 1);
	setCameraBound(Stage::stageLayout, boundX + 256, 0);
}

void setZoomAll_impl(fx32 zoom, u32 delay, u8 _playerID, u8 unk)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
		Stage::setZoom(zoom, delay, playerID, unk);
}

ncp_thumb void drawStageIntroLuigiHead()
{
	u32 lives = Game::getPlayerLives(1);

	GXOamAttr* luigiHeadIcon = rcast<GXOamAttr*>(0x0216F01C);
	GXOamAttr** digits = rcast<GXOamAttr**>(0x0216CCA8);

	GXOamAttr attrs[7];
	MI_CpuCopy8(luigiHeadIcon, &attrs, 0x38u);

	attrs[0].attr2 = attrs[0].attr2 & 0xFC00 | digits[lives % 10]->attr2 & 0x3FF;
	if ( lives / 10 <= 0 )
		attrs[1].attr1 = attrs[1].attr1 & 0xFE00 | 0x100;
	else
		attrs[1].attr2 = attrs[1].attr2 & 0xFC00 | digits[lives / 10]->attr2 & 0x3FF;

	UI::draw(34, attrs, OAM::Flags::None, 0, 0, nullptr, 0, nullptr, OAM::Settings::None, 0, 0);
}

// ======================================= ENTRANCE POSITIONING =======================================

ncp_asmfunc void forceLuigiSpawnSameEntrance_ASM()
{asm(R"(
// Force Luigi to spawn in the same entrance as Mario
ncp_jump(0x0215E5A4, 54)
	LDRB    R5, [R1,#8]
	STRB    R5, [R1,#9]
	B       0x0215E5A8

ncp_jump(0x0215EFF0, 54)
	LDRB    R2, [R1,#8]
	STRB    R2, [R1,#9]
	B       0x0215EFF4
)");}

// Player positioning on multiplayer entrance spawn
ncp_thumb void adjustEntrancePosition()
{
	if (Game::getPlayerCount() == 1 || getAlivePlayerID() != 2)
		return;

	s8 entranceID = rcast<s8*>(0x020CA8F4)[8];
	StageEntrance* entrance = Entrance::getEntrance(entranceID);
	Vec3* entranceVecs = rcast<Vec3*>(0x020CA928);

	switch (entrance->type)
	{
	// Pipe
	case EntranceType::PipeUp:
	case EntranceType::PipeDown:
		entranceVecs[0].x += 4 << 12;
		entranceVecs[1].x -= 4 << 12;
	case EntranceType::PipeLeft:
	case EntranceType::PipeRight:
		break;
	// Climbing Vine
	case EntranceType::Vine:
		entranceVecs[1].y -= 16 << 12;
		break;
	// Any other entrance
	default:
		entranceVecs[0].x += 8 << 12;
		entranceVecs[1].x -= 8 << 12;
		break;
	}
}

ncp_repl(0x0215E914, 54, "MOV R1, R6")

ncp_asmfunc void adjustEntrancePositionCall_ASM()
{asm(R"(
ncp_jump(0x0215E924, 54)
	BL      _ZN9CoopStage22adjustEntrancePositionEv
	B       0x0215E958
)");}

// Center vine head
ncp_repl(0x0211C218, 10, "MOV R0, R4")

ncp_call(0x0211C21C, 10)
ncp_thumb void call_0211C21C_ov10(Player* player)
{
	s32 playerID = player->linkedPlayerID;
	if (playerID == 0 || (playerID == 1 && isPlayerDead[0]))
	{
		CoopSym_SpawnGrowingEntranceVine(&player->position);
	}
}

// Only show one door when spawning at a door entrance

ncp_call(0x02118A90, 10)
ncp_thumb void call_02118A90_ov10(Player* self, EntranceType entranceType)
{
	if (Game::getPlayerCount() != 1 && areaHasRotator())
	{
		// If the level has a rotator, players need to change
		// view at the same time, which causes them to come out of
		// the same door at the same time.
		// This adjusts the positions as if it were an area change.

		s32 playerID = self->linkedPlayerID;
		if (playerID == 0 && !isPlayerDead[1])
		{
			self->position.x += 8 << 12;
		}
		else if (playerID == 1 && !isPlayerDead[0])
		{
			self->position.x -= 8 << 12;
		}
	}

	self->beginEntrancePose(entranceType); // Keep replaced instruction
}

ncp_repl(0x0211C980, 10, "MOV R0, R4") // Pass 'this' instead of 'this->door'

ncp_call(0x0211C984, 10)
ncp_thumb void call_0211C984_ov10(Player* player)
{
	// Open door normally if we didn't change area or singleplayer
	if (Game::getPlayerCount() == 1 || !(doorFromAreaChange || areaHasRotator()))
	{
		player->door->open();
		return;
	}

	s32 playerID = player->linkedPlayerID;
	if (playerID == 0 || (playerID == 1 && isPlayerDead[0]))
	{
		Door* door = player->door;

		if (playerID == 0 && !isPlayerDead[1])
			door->position.x -= 8 << 12;

		if (playerID == 1)
			doorFromAreaChange = false;

		door->open();
		return;
	}

	// Only playerID 1 will ever reach here

	// Do not render door opening
	player->physicsFlag.standardDoorTransit = false;
	doorFromAreaChange = false;
}

// Do not allow entering doors at the same time

ncp_call(0x0211EB24, 10)
bool call_0211EB24_ov10(Player* player)
{
	if (!player->getGroundPoundCancelKeyHeldEx()) // Keep replaced instruction
		return false;

	if (Game::getPlayerCount() == 1)
		return true;

	s32 playerID = player->linkedPlayerID;
	s32 otherID = playerID ^ 1;

	Player* other = Game::getPlayer(otherID);
	return (!other->physicsFlag.standardDoorTransit && !other->physicsFlag.bossDoorTransit);
}

// ======================================= RESPAWN =======================================

ncp_set_call(0x021041F4, 10, Coop::isMultiplayer)
ncp_set_call(0x0212B318, 11, Coop::isMultiplayer)

ncp_repl(0x02119CB8, 10, "NOP") // Do not freeze timer on player death (so we can control ourselves)

ncp_call(0x02119CC0, 10)
ncp_thumb void Player_freezeTimerOnDeathHook(s32 playerID, bool dead)
{
	Game::setPlayerDead(playerID, dead); // Keep replaced instruction

	DesyncGuard::markDesyncCheck();

	for (u32 iPlayerID = 0; iPlayerID < Game::getPlayerCount(); iPlayerID++)
	{
		if (!Game::getPlayerDead(iPlayerID))
			return;
	}
	u8& isTimeStopped = *rcast<u8*>(0x020CA898);
	isTimeStopped |= 0x48;
}

ncp_thumb bool isRespawnAllowed(Player* player, Player* other)
{
	return
		Game::getPlayerLives(player->linkedPlayerID) != 0 &&
		!Game::getPlayerDead(other->linkedPlayerID) &&
		!other->transitioningFlag &&
		!isBossFight() &&
		!hasLevelFinished();
}

THUMB_BARRIER_FUNCTION

ncp_thumb bool playerDeadState(Player* player, void* arg)
{
	constexpr u32 TextStayTime = 90;
	constexpr u32 TextHideTime = 90;

	u32 playerID = player->linkedPlayerID;

	s8& step = player->transitionStateStep;
	s16& timer = player->transitStepTimer;
	if (step == Func::Init)
	{
		step = 1;
		timer = TextHideTime;

		player->visible = false;
		*rcast<u8*>(0x020CA880) &= ~0x10; // Restore pausing

		if (playerID == Game::localPlayerID)
		{
			FS::loadFileLZ77(spectateTextFileID, (u16*)HW_OBJ_VRAM); // Overwrite StageFX VRAM
		}

		return true;
	}
	if (step == Func::Exit)
	{
		return true;
	}

	u32 otherID = playerID ^ 1;

	Player* other = Game::getPlayer(otherID);

	// Always match the spectating player's position
	player->position.x = other->position.x;
	player->position.y = other->position.y;

	CoopSym_Stage_zoom[playerID] = CoopSym_Stage_zoom[otherID];

	// Check if player is allowed to respawn or not
	if (isRespawnAllowed(player, other))
	{
		timer++;
		if (timer > TextHideTime)
		{
			if (playerID == Game::localPlayerID)
			{
				GXOamAttr* stageFxAttrs = rcast<GXOamAttr*>(0x0216F6C8);
				u8 palette = Game::getPlayerCharacter(playerID) ? 3 : 1;
				OAM::draw(stageFxAttrs, 128, 180, OAM::Flags::None, palette, 0, OAM::Settings::None);
			}
			if (timer > TextStayTime + TextHideTime)
				timer = 0;
		}

		if (player->getJumpKeyPressed())
		{
			player->position.x = other->position.x - 0x10000;
			player->position.y = other->position.y;

			player->spawnDefault();

			Particle::Handler::createParticle(249, player->position);
			Particle::Handler::createParticle(250, player->position);

			player->visible = true;
			CoopCamera::setFollowTarget(playerID, playerID);
			isPlayerDead[playerID] = false;
			Game::setPlayerDead(playerID, false);
		}
	}

	return true;
}

ncp_thumb void beginPlayerSpectate(u32 playerID)
{
	CoopCamera::setFollowTarget(playerID, playerID ^ 1);
	isPlayerDead[playerID] = true;
	Game::setPlayerDead(playerID, true);
}

void switchToPlayerSpectateState(Player* player)
{
	player->switchMainState(&Player::idleState);
	player->switchTransitionState(ptmf_cast(playerDeadState));
}

ncp_thumb bool customPlayerCreateCase(Player* player)
{
	u32 playerID = player->linkedPlayerID;
	if (Game::getPlayerLives(playerID) == 0 || isPlayerDead[playerID])
	{
		player->damageCooldown = 35; // Do not die when spawning (eg.: to Pipe Piranhas)
		beginPlayerSpectate(player->linkedPlayerID);
		switchToPlayerSpectateState(player);
		return true;
	}
	return false;
}

ncp_thumb bool customRespawnCondition(u32 playerID, s32 lives)
{
	// Prevent lives going negative
	if (lives < 0)
		Game::setPlayerLives(playerID, 0);

	u32 otherID = playerID ^ 1;
	if (Game::getPlayerDead(otherID))
	{
		Game::fader.fadeMaskShape[otherID] = scast<u8>(FadeMask::Shape::Bowser); // Other player also gets the Bowser death screen
		Stage::exitLevel();
		return false;
	}
	return true;
}

ncp_call(0x02118968, 10)
ncp_thumb void customRespawnReset(Player* player)
{
	player->reset(); // Keep replaced instruction
	beginPlayerSpectate(player->linkedPlayerID);
}

ncp_call(0x02118DE4, 10)
ncp_thumb void customPlayerRespawnCreateCase(Player* player)
{
	switchToPlayerSpectateState(player);

	u32 seqID = Entrance::getSpawnMusic(player->linkedPlayerID);
	SND::playStageBGM(seqID);
}

ncp_asmfunc void customRespawnConditionCall_ASM()
{asm(R"(
// Do not allow player to respawn so we can control it ourselves
ncp_jump(0x0212B334, 11)
	MOV     R0, R6
	MOV     R1, R4
	BL      _ZN9CoopStage22customRespawnConditionEml
	B       0x0212B33C

// Add custom player create case to prevent it from spawning if dead or spectating
ncp_jump(0x020FFB4C, 10)
	MOV     R0, R5
	BL      _ZN9CoopStage22customPlayerCreateCaseEP6Player
	CMP     R0, #0
	BNE     0x020FFD7C
	CMP     R4, #0x14
	B       0x020FFB50
)");}

ncp_call(0x02118980, 10)
ncp_thumb Vec3 customRespawnEntrance(u8 playerID)
{
	u32 otherID = playerID ^ 1;
	Player* other = Game::getPlayer(otherID);

	Entrance::transitionFlags[playerID] = Entrance::transitionFlags[otherID] & EntranceTransitionFlags::SubScreen;
	Entrance::overrideSpawnPosition(playerID, other->position.x, other->position.y);

	return Entrance::accessSpawnEntrance(playerID);
}

ncp_thumb void customTransitEntranceSpawn(Player* player, EntranceType entranceType)
{
	if (isPlayerDead[player->linkedPlayerID])
	{
		switchToPlayerSpectateState(player);
		return;
	}
	player->transitEntranceSpawn(entranceType);
}

ncp_set_call(0x02118D0C, 10, customTransitEntranceSpawn)
ncp_set_call(0x02118D98, 10, customTransitEntranceSpawn)
ncp_set_call(0x02118DFC, 10, customTransitEntranceSpawn)

ncp_asmfunc void skipViewReloadOnRespawn_ASM()
{asm(R"(
// Skip view reload on respawn if there is autoscroll
ncp_jump(0x021189BC, 10)
	LDR     R0, =0x020CACD4
	LDRB    R0, [R0]
	CMP     R0, #0
	BNE     0x02118AFC
	B       0x02118A34
)");}

ncp_asmfunc void Player_beginCutscene_SUPER(Player* self, bool lookAtBoss)
{asm(R"(
	PUSH    {R4,R5,LR}
	B       0x0211F350
)");}

ncp_asmfunc void Player_endCutscene_SUPER(Player* self)
{asm(R"(
	PUSH    {R4,LR}
	B       0x0211F2F0
)");}

ncp_jump(0x0211F34C, 10)
void Player_beginCutscene_OVERRIDE(Player* self, bool lookAtBoss)
{
	if (!Game::playerDead[self->linkedPlayerID])
		Player_beginCutscene_SUPER(self, lookAtBoss);
}

ncp_jump(0x0211F2EC, 10)
void Player_endCutscene_OVERRIDE(Player* self)
{
	if (!Game::playerDead[self->linkedPlayerID])
		Player_endCutscene_SUPER(self);
}

ncp_call(0x02118AF8, 10)
ncp_thumb void Player_viewTransitState_respawnViewSetup(StageLayout* self, Vec3* focusPos, s32 playerID, u32 isFromStageOnCreate)
{
	CoopSym_StageLayout_setupView(self, focusPos, playerID, isFromStageOnCreate);

	if (Game::getPlayerCount() == 1)
		return;

	// Restore the camera bound limits on respawn

	u32 otherID = playerID ^ 1;

	Player* player = Game::getPlayer(playerID);
	Player* other = Game::getPlayer(otherID);

	if (isPlayerDead[playerID] && player->viewID == other->viewID)
	{
		u8* rawStageLayout = rcast<u8*>(Stage::stageLayout);

		*rcast<fx32*>(&rawStageLayout[20 * playerID + 0xA900]) = *rcast<fx32*>(&rawStageLayout[20 * otherID + 0xA900]);
		*rcast<fx32*>(&rawStageLayout[20 * playerID + 0xA8FC]) = *rcast<fx32*>(&rawStageLayout[20 * otherID + 0xA8FC]);
		*rcast<fx32*>(&rawStageLayout[20 * playerID + 0xA8F8]) = *rcast<fx32*>(&rawStageLayout[20 * otherID + 0xA8F8]);
		*rcast<fx32*>(&rawStageLayout[20 * playerID + 0xA8F4]) = *rcast<fx32*>(&rawStageLayout[20 * otherID + 0xA8F4]);
	}
}

/*
//Player can't respawn when switching areas
void hook_0215EB28_ov_36()
{
	for (int i = 0; i < GetPlayerCount(); i++)
	{
		PlayerActor* player = GetPtrToPlayerActorByID(i);
		if (player)
		{
			if (player_wasDead[i] || GetLivesForPlayer(i) == 0)
			{
				SetupRespawnLocationForPlayer(i);
				PlayerActor_setEntranceState(player, 0x0211870C, 0); //Call respawn system (Forces entrance reload)
			}
		}
	}
}

void hook_02006ADC()
{
	player_wasDead[0] = 0;
	player_wasDead[1] = 0;
}

//Only freeze timer and pause menu on toad houses
void nsub_0212B908_ov_0B(u8* player)
{
	if (*(int*)0x02085A18 == 8 || GetPlayerCount() == 1)
	{
		*(int*)0x020CA898 |= 0x40;
		*(int*)0x020CA880 |= 0x10;
		player[1968] = 1;
		player[454] |= 1;
	}
}*/

// ======================================= MISC =======================================

ncp_thumb void defaultOnLevelLoad()
{
	CoopCamera::clearSpectators();

	isPlayerDead[0] = false;
	isPlayerDead[1] = false;

	flagpoleLinkedPlayer = nullptr;
	playerInFlagpoleSentFlying[0] = false;
	playerInFlagpoleSentFlying[1] = false;
}

ncp_thumb void defaultOnLayoutCreate()
{
	CoopCamera::onStageLayoutCreate();

	EntranceType entranceType = Entrance::spawnEntrance[0]->type;
	doorFromAreaChange =
		entranceType == EntranceType::Door ||
		entranceType == EntranceType::Unknown14 ||
		entranceType == EntranceType::Unknown15;

#ifdef COOP_CUSTOM_WORLD_UNLOCK
	// Mini-mushroom cutscene
	if (CoopFlagActor::findByAction(
		CoopFlagActor::Action::WorldUnlockCutscene))
	{
		CoopCamera::clearSpectators();

		*rcast<u32*>(0x02085ACC) |= 0x20; // toadHouseFlag
		*rcast<u32*>(0x020CA8B4) = 0x1000; // timeLeft
		*rcast<u8*>(0x020CA898) = 0x21; // timeStopped
	}
#endif

	forcedAreaReloadsBlocked = false;

#ifdef COOP_FIX_LOOPER
	rcast<u8*>(0x020CACB4)[Game::localPlayerID] = 0; // Reset looper
#endif

#ifdef COOP_FIX_LIQUID
	CoopFixes::Liquid::reset();
#endif

#ifdef COOP_FIX_VOLCANO_ERUPTION
	CoopFixes::VolcanoEruption::reset();
#endif
}

void defaultOnLayoutUpdate()
{
#ifdef COOP_FIX_VOLCANO_ERUPTION
	u32& levelEndBitmask = *rcast<u32*>(0x020CA8C0);
	if ((levelEndBitmask & 3) == 0 && getFgScreenID(Game::localPlayerID) == 15)
	{
		CoopFixes::VolcanoEruption::updateBackground();
	}
#endif
}

// Disable background HDMA parallax
ncp_repl(0x020AECAC, 0, "CMP R1, #1")
ncp_repl(0x020AED90, 0, ".int _ZN4Game11playerCountE")

ncp_set_call(0x020BD820, 0, Game::getPlayerCount) // Bottom screen background draw
ncp_set_call(0x020BDA90, 0, Game::getPlayerCount) // Bottom screen background execute
ncp_set_call(0x020BDC1C, 0, Game::getPlayerCount) // Bottom screen background load

ncp_asmfunc void updateHudCoins_ASM()
{asm(R"(
// Draw Luigi's HUD with Mario's values (shared coins)
ncp_jump(0x020A3578, 0)
	LDR     R0, =_ZN4Game6vsModeE
	LDRB    R0, [R0]
	CMP     R0, #0
	MOVEQ   R0, #0
	LDRNE   R0, [R1]
	B       0x020A357C
)");}

ncp_repl(0x020C03F4, 0, "MOV R0, #0") // Display Mario's score instead of local player score

// Hide dead player
ncp_call(0x020BE5C4, 0)
bool call_020BE5C4_ov0(u32 playerID)
{
	return Game::getPlayer(playerID) && !isPlayerDead[playerID];
}

ncp_repl(0x020BED88, 0, "NOP") // Do not draw singleplayer player position indicators on progress bar

ncp_repl(0x020BE5AC, 0, "PUSH {R4-R9,LR}")
ncp_repl(0x020BE5D0, 0, "POPEQ {R4-R9,PC}")
ncp_repl(0x020BE5E4, 0, "POPLT {R4-R9,PC}")
ncp_repl(0x020BE668, 0, "POP {R4-R9,PC}")

ncp_asmfunc void drawMvsLProgressBar_ASM()
{asm(R"(
ncp_jump(0x020BE5B4, 0)
	LDR     R9, =_ZN4Game6vsModeE
	LDRB    R9, [R9]
	MOV     R6, R1 // Keep replaced instruction
	B       0x020BE5B8

// MvsL progress bar uses singleplayer pixel scale
ncp_jump(0x020BE5E8, 0)
	CMP     R9, #0
	MOVEQ   R0, #212
	MOVNE   R0, #209
	B       0x020BE5EC

// MvsL progress bar uses singleplayer OAM y_shift
ncp_jump(0x020BE610, 0)
	CMP     R9, #0
	MOVEQ   R8, #6
	MOV     R0, R6 // Keep replaced instruction
	B       0x020BE614

// MvsL progress bar uses singleplayer OAM addresses
ncp_jump(0x020BE650, 0)
	CMP     R9, #0
	LDREQ   R1, =0x020CA104
	STR     R8, [SP,#0x18] // Keep replaced instruction
	B       0x020BE654

// MvsL progress bar uses singleplayer BNCL rectangle index
ncp_jump(0x020BE658, 0)
	CMP     R9, #0
	MOVEQ   R0, #7
	MOVNE   R0, R4
	B       0x020BE65C

// Draw MvsL progress bar instead of singleplayer
ncp_jump(0x020BF124, 0)
	MOV     R1, #0
	MOV     R2, #0
	BL      0x020BE674 // Draw the multiplayer one
	MOV     R0, R4
	BL      0x020BECC4 // Draw the singleplayer one
	B       0x020BF128
)");}

// Draw bottom screen lives my way
ncp_call(0x020BF12C, 0)
void call_020BF12C_ov0()
{
	if (Game::getPlayerCount() == 1)
	{
		CoopSym_DrawBottomScreenLives();
		return;
	}

	GXOamAttr** liveCounterForPlayer_1P = rcast<GXOamAttr**>(0x020CA00C);
	s32 xShift = *rcast<s32*>(0x020CC2C4);

	UI::drawSub(6, liveCounterForPlayer_1P[0], OAM::Flags::None, 0, 0, 0, 0, 0, OAM::Settings::None, -xShift - 64 - 4, 0);
	UI::drawSub(6, liveCounterForPlayer_1P[1], OAM::Flags::None, 0, 0, 0, 0, 0, OAM::Settings::None, -xShift + 4, 0);
}

// Update lives counter for both players
ncp_repl(0x020C0434, 0, "LDRB R0, [R0,R6]")
ncp_repl(0x020C0790, 0, ".int _ZN4Game15playerCharacterE")

// Load tilemap with background for 2 life counters
ncp_call(0x020C0A34, 0)
void call_020C0A34_ov0(u32 extFileID, void* dest)
{
	if (Game::getPlayerCount() != 1)
		extFileID = "coop/d_2d_UI_O_1P_game_in_d_nsc.bin"fid;
	FS::loadFileLZ77(extFileID, dest);
}

ncp_set_call(0x020D8E70, 10, Coop::isNotVS) // Disable MvsL coin score (on collected)
ncp_set_call(0x020D9C24, 10, Coop::isNotVS) // Disable MvsL coin score (on block hit)
ncp_set_call(0x020D3348, 10, Coop::isNotVS) // Disable MvsL coin score for coin actor

// Allow score incrementation
ncp_set_call(0x0209AC1C, 0, Coop::isNotVS)
ncp_repl(0x0209AC20, 0, "CMP R0, #0")

// Allow score incrementation from actors
ncp_set_call(0x0209ABA8, 0, Coop::isNotVS)
ncp_repl(0x0209ABAC, 0, "CMP R0, #0")

ncp_repl(0x02020300, "MOV R0, #0; NOP") // All score goes to Mario

ncp_asmfunc void sharedAddCoins_ASM()
{asm(R"(
// Share player coins (all coins go to Mario)
ncp_jump(0x02020358)
	LDR     R1, =_ZN4Game6vsModeE
	LDRB    R1, [R1]
	CMP     R1, #0
	MOVEQ   R0, #0
	MOV     R4, R0
	B       0x02020370
)");}

ncp_call(0x020203EC)
void call_020203EC() // When Mario gets 1-up from coins, also give Luigi 1-up.
{
	for (s32 i = 0; i < Game::getPlayerCount(); i++)
		StageEntity::getCollectablePoints(8, i);
}

ncp_asmfunc void entityPersistence_ASM()
{asm(R"(
check_isNotVs_R1:
	LDR     R1, =_ZN4Game6vsModeE
	LDRB    R1, [R1]
	CMP     R1, #0
	BX      LR

// Powerups don't despawn
ncp_jump(0x020D13B4, 10)
	BL      check_isNotVs_R1
	BLNE    0x020D4D1C
	B       0x020D13B8

// Permanently destroyed entities do not respawn
ncp_jump(0x0209B7C0, 0)
	BL      check_isNotVs_R1
	BLNE    0x0209C288
	B       0x0209B7C4
)");}

// WARNING: Different water heights between views in the same area WILL BREAK. (Forced area reload is the fix)

// Make areas always reload if the area number is not 0
// Or if there are liquids in the level

ncp_thumb void decideForceAreaReload()
{
	if (forceAreaReload == 1) // Already set to reload
		return;

	// Prevent rotators from resetting but still get rid of the lava in W8 Final Castle
	if (forcedAreaReloadsBlocked)
		return;

	StageObject* stageObjs = Stage::stageBlocks.stageObjs;
	for (u32 i = 0; ; i++)
	{
		StageObject* stageObj = &stageObjs[i];
		u16 stageObjID = stageObj->id;
		if (stageObjID == 0xFFFF) // Array end
			break;
		if (stageObjID == 231 || stageObjID == 234 || stageObjID == 259) // Liquid
		{
			forceAreaReload = 1;
			break;
		}
	}
}

ncp_asmfunc void forceAreaReloadLogic_ASM()
{asm(R"(
// Force reload if destination area number is not 0
ncp_call(0x0201E91C)
ncp_call(0x0201E8A0)
	STR     R0, [R1] // Keep replaced instruction
	LDR     R0, =_ZN9CoopStage15forceAreaReloadE
	MOV     R1, #1
	STRB    R1, [R0]
	BX      LR

// Force reload if extra checks say so
ncp_jump(0x0201E928)
	STRB    R1, [R0] // Keep replaced instruction
	BL      _ZN9CoopStage21decideForceAreaReloadEv
	B       0x0201E92C

ncp_jump(0x0201E8A8)
	STRB    R4, [R0] // Keep replaced instruction
	BL      _ZN9CoopStage21decideForceAreaReloadEv
	B       0x0201E8AC

// Custom variable determines if reload happens
ncp_jump(0x02119638, 10)
	LDR     R3, =_ZN9CoopStage15forceAreaReloadE
	LDRB    R2, [R3]
	CMP     R2, #0
	MOV     R2, #0
	STRB    R2, [R3]
	BNE     0x02119664
	B       0x02119640
)");}

ncp_repl(0x0215E4AC, 54, "NOP") // StageScene::setup load the area even if the same

ncp_call(0x0211E794, 10)
bool Player_updateTimesUpTransitionsHook(Player* self)
{
	// Do not kill the player that is alive when time = 0
	if (hasLevelFinished())
		return false;
	return self->updateTimesUpTransitions();
}

ncp_asmfunc void preventFreezingFlagOnLevelLoad_ASM()
{asm(R"(
// Prevent freezing flag being set on level load
ncp_jump(0x02006AA0)
	LDR     R0, =_ZN4Game11playerCountE
	B       0x02006AA4

ncp_over(0x02006AB0)
	CMP     R0, #1
	MOVGT   R0, #1
	MOVLE   R0, #0
ncp_endover()
)");}

// Prevent particle handler from always updating in VS mode
// This should allow Mario Vs Luigi mods to freeze the particles if they want
ncp_repl(0x02022C50, "B 0x02022C7C")

// Only disable the particle handler on transitions and powerup change in singleplayer
ncp_call(0x021207F0, 10)
ncp_thumb void customDisableParticlesOnTransit()
{
	if (Game::getPlayerCount() == 1)
		Particle::Handler::disable();
}

ncp_repl(0x02119CBC, 10, "NOP") // Do not freeze camera on death

ncp_repl(0x0212B930, 11, "NOP") // Do not freeze time on transitions

// Spawn actors for both players on setup

ncp_asmfunc void spawnActorsForBothPlayers_ASM()
{asm(R"(
ncp_over(0x0209C4A4, 0)
	CMP     R0, #1
	BEQ     0x0209C50C
	NOP
	NOP
ncp_endover()
)");}

ncp_call(0x0209C4E0, 0)
void customSpawnBattleStarRoot(u16 objectID, u32 settings, const Vec3* position, const Vec3s* rotation, const fx32* scale, const s8* linkPlayerID)
{
	if (!Coop::isActive)
		Actor::spawnActor(objectID, settings, position, rotation, scale, linkPlayerID);
}

// Use Game::playerCount instead of Game::vsMode in spawnActorsForBothPlayers_ASM
ncp_repl(0x0209C584, 0, ".int _ZN4Game11playerCountE")

// Do not allow entering entrances if flagpole in use

ncp_thumb EntranceUseResult Entrance_customTryUseEntrance(fx32 x, fx32 y, u8 playerID)
{
	if (isFlagpoleGrabbed())
		return EntranceUseResult::InvalidEntrance;

	return Entrance::tryUseEntrance(x, y, playerID);
}

ncp_set_call(0x020A77D0, 0, Entrance_customTryUseEntrance)
ncp_set_call(0x020A79D4, 0, Entrance_customTryUseEntrance)
ncp_set_call(0x020A7BD8, 0, Entrance_customTryUseEntrance)
ncp_set_call(0x020A7DB0, 0, Entrance_customTryUseEntrance)

// Do not show "TOUCH" if the player is dead

ncp_call(0x020BE1F0, 0)
u32 Stage_customPowerupDeployCondition(u32 playerID)
{
	return Game::getPlayerInventoryPowerup(playerID) && // Keep replaced instruction
		!Game::getPlayerDead(playerID);
}

// Save on 3D memory, required for boss fights

ncp_call(0x0215E850, 54)
ncp_thumb void doNotLoadDoorModels()
{
	if (CoopFlagActor::findByAction(
		CoopFlagActor::Action::DoNotLoadDoorModels))
		return;
	Door::loadModels();
}

ncp_call(0x020AF2E4, 0)
ncp_thumb void doNotLoadCastleModel()
{
	if (CoopFlagActor::findByAction(
		CoopFlagActor::Action::DoNotLoadCastleModel))
		return;
	CoopSym_loadCastleModel();
}

} // namespace CoopStage
