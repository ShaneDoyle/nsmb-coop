#include "Stage.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/game/sound/sound.hpp>
#include <nsmb/game/player.hpp>
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

#include "PlayerSpectate.hpp"

NTR_USED static u32 sTempVar;

u8 Stage_isPlayerDead[2];
u8 Stage_doorFromAreaChange;

asm(R"(
	SpawnGrowingEntranceVine = 0x020D0CEC
	_ZN5Stage9exitLevelEm = 0x020A189C
	_ZN5Stage4zoomE = 0x020CADB4
	StageLayout_looperScrollBack = 0x020B1510
)");
extern "C" {
	void SpawnGrowingEntranceVine(Vec3*);
	void StageLayout_looperScrollBack(void* stageLayout, s32 playerID);
}
namespace Stage {
	void exitLevel(u32 flag);
	extern fx32 zoom[2];
}

// ======================================= GETTERS =======================================

static bool Stage_getLocalPlayerID() { return Game::localPlayerID; }
static bool Stage_getLuigiMode() { return Game::luigiMode; }
static bool Stage_getMultiplayer() { return Game::getPlayerCount() > 1; }

static s32 Stage_getAlivePlayerID()
{
	if (Stage_isPlayerDead[0])
		return 1;
	if (Stage_isPlayerDead[1])
		return 0;
	return 2; // both
}

// ======================================= ENTRANCE POSITIONING =======================================

asm(R"(
// Force Luigi to spawn in the same entrance as Mario
ncp_jump(0x0215E5A4, 54)
	LDRB    R5, [R1,#8]
	STRB    R5, [R1,#9]
	B       0x0215E5A8

ncp_jump(0x0215EFF0, 54)
	LDRB    R2, [R1,#8]
	STRB    R2, [R1,#9]
	B       0x0215EFF4
)");

// Player positioning on multiplayer entrance spawn
NTR_USED static void Stage_adjustEntrancePosition()
{
	if (Game::getPlayerCount() == 1 || Stage_getAlivePlayerID() != 2)
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

asm(R"(
ncp_jump(0x0215E924, 54)
	BL      _ZL28Stage_adjustEntrancePositionv
	B       0x0215E958
)");

// Center vine head
ncp_repl(0x0211C218, 10, "MOV R0, R4")

ncp_call(0x0211C21C, 10)
void call_0211C21C_ov10(Player* player)
{
	s32 playerID = player->linkedPlayerID;
	if (playerID == 0 || (playerID == 1 && Stage_isPlayerDead[0]))
	{
		SpawnGrowingEntranceVine(&player->position);
	}
}

// Only show one door when spawning at a door entrance

ncp_repl(0x0211C980, 10, "MOV R0, R4") // Pass 'this' instead of 'this->door'

ncp_call(0x0211C984, 10)
void call_0211C984_ov10(Player* player)
{
	// Open door normally if we didn't change area or singleplayer
	if (Game::getPlayerCount() == 1 || !Stage_doorFromAreaChange)
	{
		player->door->open();
		return;
	}

	s32 playerID = player->linkedPlayerID;
	if (playerID == 0 || (playerID == 1 && Stage_isPlayerDead[0]))
	{
		Door* door = player->door;

		if (playerID == 0 && !Stage_isPlayerDead[1])
			door->position.x -= 8 << 12;

		if (playerID == 1)
			Stage_doorFromAreaChange = false;

		door->open();
		return;
	}

	// Only playerID 1 will ever reach here

	// Do not render door opening
	player->physicsFlag.standardDoorTransit = false;
	Stage_doorFromAreaChange = false;
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

ncp_set_call(0x021041F4, 10, Stage_getMultiplayer)
ncp_set_call(0x0212B318, 11, Stage_getMultiplayer)
ncp_repl(0x02119CB8, 10, "NOP") // Do not freeze timer on player death


static bool Stage_playerDeadState(Player* player, void* arg)
{
	u32 playerID = player->linkedPlayerID;

	s8& step = player->transitionStateStep;
	if (step == Func::Init)
	{
		step = 1;

		player->visible = false;

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

	Stage::zoom[playerID] = Stage::zoom[otherID];

	// Check if player is allowed to respawn or not
	if (player->getJumpKeyPressed() &&
		Game::getPlayerLives(playerID) != 0 &&
		!Game::getPlayerDead(otherID) &&
		!Stage_isBossFight() &&
		!Stage_hasLevelFinished())
	{
		player->position.x = other->position.x - 0x10000;
		player->position.y = other->position.y;

		player->spawnDefault();

		Particle::Handler::createParticle(249, player->position);
		Particle::Handler::createParticle(250, player->position);

		player->visible = true;
		PlayerSpectate::setTarget(playerID, playerID);
		Stage_isPlayerDead[playerID] = false;
		Game::setPlayerDead(playerID, false);
	}

	return true;
}

static void Stage_beginPlayerSpectate(u32 playerID)
{
	PlayerSpectate::setTarget(playerID, playerID ^ 1);
	Stage_isPlayerDead[playerID] = true;
	Game::setPlayerDead(playerID, true);
}

static void Stage_switchToPlayerSpectateState(Player* player)
{
	player->switchMainState(&Player::idleState);
	player->switchTransitionState(ptmf_cast(Stage_playerDeadState));
}

NTR_USED static bool Stage_customPlayerCreateCase(Player* player)
{
	u32 playerID = player->linkedPlayerID;
	if (Game::getPlayerLives(playerID) == 0 || Stage_isPlayerDead[playerID])
	{
		Stage_beginPlayerSpectate(player->linkedPlayerID);
		Stage_switchToPlayerSpectateState(player);
		return true;
	}
	return false;
}

NTR_USED static bool Stage_customRespawnCondition(u32 playerID, s32 lives)
{
	u32 otherID = playerID ^ 1;
	if (Game::getPlayerDead(otherID))
	{
		Game::fader.fadeMaskShape[otherID] = scast<u8>(FadeMask::Shape::Bowser); // Other player also gets the Bowser death screen
		Stage::exitLevel(0);
		return false;
	}
	return true;
}

ncp_call(0x02118968, 10)
static void Stage_customRespawnReset(Player* player)
{
	player->reset(); // Keep replaced instruction
	Stage_beginPlayerSpectate(player->linkedPlayerID);
}

ncp_call(0x02118DE4, 10)
static void Stage_customPlayerRespawnCreateCase(Player* player)
{
	Stage_switchToPlayerSpectateState(player);

	u32 seqID = Entrance::getSpawnMusic(player->linkedPlayerID);
	SND::playStageBGM(seqID);
}

asm(R"(
// Do not allow player to respawn so we can control it ourselves
ncp_jump(0x0212B334, 11)
	MOV     R0, R6
	MOV     R1, R4
	BL      _ZL28Stage_customRespawnConditionml
	B       0x0212B33C

// Add custom player create case to prevent it from spawning if dead or spectating
ncp_jump(0x020FFB4C, 10)
	MOV     R0, R5
	BL      _ZL28Stage_customPlayerCreateCaseP6Player
	CMP     R0, #0
	BNE     0x020FFD7C
	CMP     R4, #0x14
	B       0x020FFB50
)");

ncp_call(0x02118980, 10)
Vec3 Stage_customRespawnEntrance(u8 playerID)
{
	u32 otherID = playerID ^ 1;
	Player* other = Game::getPlayer(otherID);

	Entrance::transitionFlags[playerID] = Entrance::transitionFlags[otherID] & EntranceTransitionFlags::SubScreen;
	Entrance::overrideSpawnPosition(playerID, other->position.x, other->position.y);

	return Entrance::accessSpawnEntrance(playerID);
}

void Stage_customTransitEntranceSpawn(Player* player, EntranceType entranceType)
{
	if (Stage_isPlayerDead[player->linkedPlayerID])
	{
		Stage_switchToPlayerSpectateState(player);
		return;
	}
	player->transitEntranceSpawn(entranceType);
}

ncp_set_call(0x02118D0C, 10, Stage_customTransitEntranceSpawn)
ncp_set_call(0x02118D98, 10, Stage_customTransitEntranceSpawn)
ncp_set_call(0x02118DFC, 10, Stage_customTransitEntranceSpawn)

asm(R"(
// Skip view reload on respawn if there is autoscroll
ncp_jump(0x021189BC, 10)
	LDR     R0, =0x020CACD4
	LDRB    R0, [R0]
	CMP     R0, #0
	BNE     0x02118AFC
	B       0x02118A34
)");

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

// ======================================= PAUSE =======================================

asm(R"(
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
)");

// ======================================= LOOPER =======================================

ncp_repl(0x020BBD64, 0, "NOP") // Moved to StageLayout_onCreateHook

NTR_USED static void StageLayout_customLooperScrollBack(void* stageLayout)
{
	u8* looperApplyLoop = rcast<u8*>(0x020CACDC);

	for (u32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		if (looperApplyLoop[playerID])
			StageLayout_looperScrollBack(stageLayout, playerID);
	}
}

asm(R"(
ncp_over(0x020AE8F0, 0)
	MOV     R0, R6
	BL      _ZL34StageLayout_customLooperScrollBackPv
	B       0x020AE90C
ncp_endover()
)");

// ======================================= MISC =======================================

ncp_call(0x02006B28)
void Stage_loadLevelHook(const void* pSrc, u32 offset, u32 szByte)
{
	GX_LoadBGPltt(pSrc, offset, szByte); // Keep replaced instruction

	PlayerSpectate::onLoadLevel();

	Stage_isPlayerDead[0] = false;
	Stage_isPlayerDead[1] = false;
}

ncp_call(0x020BB7DC, 0)
void StageLayout_onCreateHook(s32 seqID)
{
	SND::stopRequestedBGM(seqID); // Keep replaced instruction

	PlayerSpectate::onStageLayoutCreate();

	rcast<u8*>(0x020CACB4)[Game::localPlayerID] = 0;

	EntranceType entranceType = Entrance::spawnEntrance[0]->type;
	Stage_doorFromAreaChange =
		entranceType == EntranceType::Door ||
		entranceType == EntranceType::Unknown14 ||
		entranceType == EntranceType::Unknown15;
}

void StageLayout_onUpdateHook()
{
	PlayerSpectate::onStageLayoutUpdate();
}

asm(R"(
ncp_jump(0x020BAC24, 0)
	BL      _Z24StageLayout_onUpdateHookv
	LDR     R0, =0x020CA850
	B       0x020BAC28
)");

ncp_repl(0x020AECA4, 0, "MOV R1, #1") // Disable background HDMA parallax

ncp_set_call(0x020BD820, 0, Game::getPlayerCount) // Bottom screen background draw
ncp_set_call(0x020BDA90, 0, Game::getPlayerCount) // Bottom screen background execute
ncp_set_call(0x020BDC1C, 0, Game::getPlayerCount) // Bottom screen background load

ncp_repl(0x020A3578, 0, "MOV R0, #0") // Draw Luigi's HUD with Mario's values (shared coins)
ncp_repl(0x020C03F4, 0, "MOV R0, #0") // Display Mario's score instead of local player score

ncp_repl(0x020BE5E8, 0, "MOV R0, #212") // MvsL progress bar uses singleplayer pixel scale
ncp_repl(0x020BE60C, 0, "MOV R8, #6") // MvsL progress bar uses singleplayer OAM y_shift
ncp_repl(0x020BE670, 0, ".int 0x020CA104") // MvsL progress bar uses singleplayer OAM addresses
ncp_repl(0x020BE658, 0, "MOV R0, #7") // MvsL progress bar uses singleplayer BNCL rectangle index
ncp_repl(0x020BED88, 0, "NOP") // Do not draw singleplayer player position indicators on progress bar

// Hide dead player
ncp_call(0x020BE5C4, 0)
bool call_020BE5C4_ov0(u32 playerID)
{
	return Game::getPlayer(playerID) && !Stage_isPlayerDead[playerID];
}

asm(R"(
// Draw MvsL progress bar instead of singleplayer
ncp_jump(0x020BF124, 0)
	MOV     R1, #0
	MOV     R2, #0
	BL      0x020BE674 // Draw the multiplayer one
	MOV     R0, R4
	BL      0x020BECC4 // Draw the singleplayer one
	B       0x020BF128
)");

// Draw bottom screen lives my way
ncp_call(0x020BF12C, 0)
void call_020BF12C_ov0()
{
	GXOamAttr** liveCounterForPlayer_1P = rcast<GXOamAttr**>(0x020CA00C);
	s32 xShift = *rcast<s32*>(0x020CC2C4);

	Game::drawBNCLSpriteSub(6, liveCounterForPlayer_1P[0], OAM::Flags::None, 0, 0, 0, 0, 0, OAM::Settings::None, -xShift - 64 - 4, 0);
	Game::drawBNCLSpriteSub(6, liveCounterForPlayer_1P[1], OAM::Flags::None, 0, 0, 0, 0, 0, OAM::Settings::None, -xShift + 4, 0);
}

// Update lives for both players
ncp_call(0x020C0444, 0)
void call_020C0444_ov0()
{
	GXOamAttr** entryTable_1P = rcast<GXOamAttr**>(0x0216F554);
	GXOamAttr** liveCounterForPlayer_1P = rcast<GXOamAttr**>(0x020CA00C);

	OAM::updateCounter(liveCounterForPlayer_1P[0], entryTable_1P, Game::getPlayerLives(0), 2, OAM::CounterFlags::UpdateShadow | OAM::CounterFlags::NoLeadingZeroes);
	OAM::updateCounter(liveCounterForPlayer_1P[1], entryTable_1P, Game::getPlayerLives(1), 2, OAM::CounterFlags::UpdateShadow | OAM::CounterFlags::NoLeadingZeroes);
}
ncp_repl(0x020C041C, 0, "B 0x020C0444")

ncp_repl(0x0209AAD0, 0, "BX LR") // Disable MvsL coin score
ncp_repl(0x020D3350, 10, "NOP") // Disable MvsL coin score for coin actor

ncp_repl(0x0209AC1C, 0, "MOV R0, #1") // Allow score incrementation
ncp_repl(0x0209ABA8, 0, "MOV R0, #1") // Allow score incrementation from actors

ncp_repl(0x02020300, "MOV R0, #0; NOP") // All score goes to Mario
ncp_repl(0x02020358, "MOV R4, #0; B 0x02020370") // Share player coins (all coins go to Mario)

ncp_call(0x020203EC)
void call_020203EC() // When Mario gets 1-up from coins, also give Luigi 1-up.
{
	for (s32 i = 0; i < Game::getPlayerCount(); i++)
		StageEntity::getCollectablePoints(8, i);
}

ncp_repl(0x020D13B4, 10, "NOP") // Powerups don't despawn
ncp_repl(0x0209B7C0, 0, "NOP") // Permanently destroyed entities do not respawn

// Prevent reloading resources on preCreate during multiplayer wait (Fixes fading)
static bool StageScene_preCreate(Scene* self)
{
	if (rcast<u32*>(self)[0x640C / 4])
		return true;
	return self->Scene::preCreate();
}

ncp_over(0x020C6E68, 0) const auto StageScene_preCreate_vtbl = StageScene_preCreate;

ncp_repl(0x020BC224, 0, "NOP") // Do not reset liquid position on player setup
ncp_repl(0x020BC22C, 0, "NOP") // Do not reset liquid position on player setup

ncp_call(0x020BBAD0, 0)
u16 Level_createHook() {
	// Reset liquid on area/level load
	Stage::liquidPosition[Game::localPlayerID] = -0x1000000;
	Stage::lastLiquidPosition[Game::localPlayerID] = -0x1000000;

	return Wifi::getCommunicatingConsoleCount(); // Keep replaced instruction
}

// WARNING: Different water heights between views in the same area WILL BREAK. (Forced area reload is the fix)

// Make areas always reload if the area number is not 0
// Or if there are liquids in the level

NTR_USED static u8 Stage_forceAreaReload = 0;

NTR_USED static void Stage_decideForceAreaReload()
{
	if (Stage_forceAreaReload == 1) // Already set to reload
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
			Stage_forceAreaReload = 1;
			break;
		}
	}
}

asm(R"(
// Force reload if destination area number is not 0
ncp_jump(0x0201E91C)
	STR     R0, [R1] // Keep replaced instruction
	LDR     R0, =_ZL21Stage_forceAreaReload
	MOV     R1, #1
	STRB    R1, [R0]
	B       0x0201E920

// Force reload if extra checks say so
ncp_jump(0x0201E928)
	STRB    R1, [R0] // Keep replaced instruction
	BL      _ZL27Stage_decideForceAreaReloadv
	B       0x0201E92C

// Custom variable determines if reload happens
ncp_jump(0x02119638, 10)
	LDR     R3, =_ZL21Stage_forceAreaReload
	LDRB    R2, [R3]
	CMP     R2, #0
	MOV     R2, #0
	STRB    R2, [R3]
	BNE     0x02119664
	B       0x02119640
)");

ncp_repl(0x0215E4AC, 54, "NOP") // StageScene::setup load the area even if the same

ncp_call(0x0211E794, 10)
bool Player_updateTimesUpTransitionsHook(Player* self)
{
	// Do not kill the player that is alive when time = 0
	if (Stage_hasLevelFinished())
		return false;
	return self->updateTimesUpTransitions();
}

ncp_repl(0x0211B67C, 10, "NOP") // Do not freeze other players on goal
ncp_repl(0x021305B4, 12, "NOP") // Do not freeze other players on goal

// No idea what these do
// ncp_repl(0x0209B254, 0, "MOV R0, #1")
// ncp_repl(0x0209BD2C, 0, "MOV R0, #1")
// ncp_repl(0x020D06E0, 10, "MOV R0, #1")

// Fix some bottom screen locking crap that took our time (related to wavy fading transition) :(
ncp_repl(0x0201EB1C, "B 0x0201EB40")
ncp_repl(0x0201EB4C, "NOP")

asm(R"(
// Disable baphs if player count is bigger than 1 (prevents desyncs)
ncp_jump(0x02012584)
	BL      _ZN4Game14getPlayerCountEv
	CMP     R0, #1
	BGT     0x0201258C
	LDR     R0, =0x02088B94
	B       0x02012588
)");

ncp_repl(0x020FBF60, 10, "BX LR") // Fix end of level for player that "lost the race"

ncp_set_call(0x021624C8, 54, Stage_getLocalPlayerID) // Midway point draws from local player
ncp_set_call(0x02162110, 54, Stage_getLocalPlayerID) // Midway point plays sound at local player position

ncp_repl(0x0215ED54, 54, "NOP") // Disable mega mushroom destruction counter

ncp_set_call(0x02152944, 54, Stage_getLuigiMode) // Allow Luigi lives on stage intro scene
ncp_set_call(0x0215293C, 54, Stage_getLuigiMode) // Allow Luigi head on stage intro scene

ncp_repl(0x020FBD70, 10, "NOP") // Disables "Lose" music. (End Flag & Boss)

asm(R"(
// Store Player* for SpawnEnemiesFromMegaGroundPound
ncp_jump(0x021121EC, 10)
	LDR     R0, =_ZL8sTempVar
	STR     R5, [R0]
	BL      0x0209E038 // SpawnEnemiesFromMegaGroundPound
	B       0x021121F0

// SpawnEnemiesFromMegaGroundPound
ncp_jump(0x0209E0D0, 0)
	LDR     R0, =_ZL8sTempVar
	LDR     R0, [R0]
	B       0x0209E0D4

// Pass the playerID to the drop controller settings
ncp_jump(0x0209E108, 0)
	ORR     R1, R0, #0x10000000
	LDR     R0, =_ZL8sTempVar
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
)");

// TODO: Fix Mega Mushroom destruction counter

asm(R"(
// Prevent freezing flag being set on level load
ncp_jump(0x02006AA0)
	LDR     R0, =_ZN4Game11playerCountE
	B       0x02006AA4

ncp_over(0x02006AB0)
	CMP     R0, #1
	MOVGT   R0, #1
	MOVLE   R0, #0
ncp_endover()
)");

// Prevent particle handler from always updating in VS mode
// This should allow Mario Vs Luigi mods to freeze the particles if they want
ncp_repl(0x02022C50, "B 0x02022C7C")

// Only disable the particle handler on transitions and powerup change in singleplayer
ncp_call(0x021207F0, 10)
void Stage_fixTransitParticles()
{
	if (Game::getPlayerCount() == 1)
		Particle::Handler::disable();
}

ncp_repl(0x0212B92C, 11, "NOP") // Do not freeze on transitions
ncp_repl(0x0212B930, 11, "NOP") // Do not freeze on transitions

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
asm(R"(
ncp_jump(0x020BA1C4, 0)
	MOV     R5, R3
	LDR     R3, =_ZL8sTempVar
	STR     R5, [R3]
	B       0x020BA1C8
)");

ncp_repl(0x020B8D20, 0, ".int _ZL8sTempVar")

ncp_repl(0x02119CBC, 10, "NOP") // Do not freeze camera on death
