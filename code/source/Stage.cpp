#include "nsmb/game.hpp"
#include "nsmb/sound.hpp"
#include "nsmb/player.hpp"
#include "nsmb/stage/entity.hpp"
#include "nsmb/stage/layout/entrance.hpp"
#include "nsmb/graphics/particle.hpp"
#include "nsmb/system/input.hpp"
#include "nsmb/system/function.hpp"
#include "nsmb/system/misc.hpp"
#include "nsmb/graphics/fader.hpp"
#include "nsmb/entity/scene.hpp"

#include "PlayerSpectate.hpp"
#include "util/eprintf.h"

NTR_USED static u32 sTempVar;
static u8 sPlayerSpectating[2];

asm(R"(
	SpawnGrowingEntranceVine = 0x020D0CEC
	_ZN5Stage9exitLevelEm = 0x020A189C
)");
extern "C" {
	void SpawnGrowingEntranceVine(Vec3*);
}
namespace Stage {
	void exitLevel(u32 flag);
}

static inline bool Stage_hasLevelFinished() { return *rcast<u32*>(0x020CA8C0); }

// ======================================= GETTERS =======================================

static bool Stage_getLocalPlayerID() { return Game::localPlayerID; }
static bool Stage_getLuigiMode() { return Game::luigiMode; }
static bool Stage_getMultiplayer() { return Game::getPlayerCount() > 1; }

static s32 Stage_getAlivePlayerID()
{
	if (sPlayerSpectating[0])
		return 1;
	if (sPlayerSpectating[1])
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
	if (playerID == 0 || (playerID == 1 && sPlayerSpectating[0]))
	{
		SpawnGrowingEntranceVine(&player->position);
	}
}

// ======================================= RESPAWN =======================================

ncp_set_call(0x021041F4, 10, Stage_getMultiplayer)
ncp_set_call(0x0212B318, 11, Stage_getMultiplayer)
ncp_repl(0x02119CB8, 10, "NOP") // Do not freeze timer on player death

static bool Stage_playerSpectateState(Player* player, void* arg)
{
	u32 playerID = player->linkedPlayerID;
	u32 otherID = playerID ^ 1;

	s8& step = player->transitionStateStep;
	if (step == Func::Init)
	{
		step = 1;

		player->visible = false;
		sPlayerSpectating[playerID] = true;
		PlayerSpectate::setTarget(playerID, otherID);
		Game::setPlayerDead(playerID, true);

		//Stage::stageLayout->scrollLevelDirect();

		u32 seqID = Entrance::getSpawnMusic(playerID);
		Sound::playStageBGM(seqID);

		return true;
	}
	if (step == Func::Exit)
	{
		return true;
	}

	// Check if player is allowed to respawn or not
	if (player->getJumpKeyPressed() && Game::getPlayerLives(playerID) != 0 && !Game::getPlayerDead(otherID))
	{
		Player* other = Game::getPlayer(otherID);

		player->position.x = other->position.x - 0x10000;
		player->position.y = other->position.y;

		player->spawnDefault();

		Particle::Handler::createParticle(249, player->position);
		Particle::Handler::createParticle(250, player->position);

		player->visible = true;
		sPlayerSpectating[playerID] = false;
		PlayerSpectate::setTarget(playerID, playerID);
		Game::setPlayerDead(playerID, false);
	}

	return true;
}

ncp_call(0x02118DE4, 10)
static void Stage_beginPlayerSpectate(Player* player)
{
	player->switchMainState(&Player::idleState);
	player->switchTransitionState(ptmf_cast(Stage_playerSpectateState));
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

NTR_USED static bool Stage_customPlayerCreateCase(Player* player)
{
	u32 playerID = player->linkedPlayerID;
	if (Game::getPlayerLives(playerID) == 0 || sPlayerSpectating[playerID])
	{
		Stage_beginPlayerSpectate(player);
		return true;
	}
	return false;
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

	Entrance::overrideSpawnPosition(playerID, other->position.x, other->position.y);
	Entrance::subScreenSpawn[playerID] = Entrance::subScreenSpawn[otherID];

	return Entrance::accessSpawnEntrance(playerID);
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

// ======================================= PAUSE =======================================

asm(R"(
// Fix desyncs on pause menu
ncp_jump(0x020A20E8, 0)
	LDR     R0, =0x6428
	LDRB    R0, [R5,R0] // R0 = pause menu owner
	LSLS    R0, R0, #1
	LDR     R1, =0x02087648
	ADD     R0, R1, R0 // R1 = &Input::consoleKeysRepeated[R0]
	B       0x020A20EC

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

// ======================================= MISC =======================================

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
	return Game::getPlayer(playerID) && !sPlayerSpectating[playerID];
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

	return Wifi::getConsoleCount(); // Keep replaced instruction
}

// WARNING: Different water heights between views in the same area WILL BREAK.

// Make areas always reload if the area number is not 0

NTR_USED static u8 Stage_forceAreaReload = 0;

asm(R"(
ncp_jump(0x0201E91C)
	STR     R0, [R1] // Keep replaced instruction
	LDR     R0, =_ZL21Stage_forceAreaReload
	MOV     R1, #1
	STR     R1, [R0]
	B       0x0201E920

ncp_jump(0x02119638, 10)
	LDR     R3, =_ZL21Stage_forceAreaReload
	LDR     R2, [R3]
	CMP     R2, #0
	MOV     R2, #0
	STR     R2, [R3]
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

ncp_repl(0x02006E00, "MOV R6, #0") // Clear freezing flag
ncp_repl(0x02006AE8, "NOP") // Prevent freezing flag being set on level load

// ======================================= PLAYER =======================================

ncp_repl(0x02109B30, 10, "B 0x02109B84") // Mario can not make Luigi fall on head jump
ncp_repl(0x02109A14, 10, "B 0x02109A68") // Luigi can not make Mario fall on head jump

ncp_repl(0x02109EB4, 10, "MOV R4, #1") // Mario doesn't bump with Luigi
ncp_repl(0x02109C1C, 10, "MOV R4, #1") // Luigi doesn't bump with Mario

ncp_call(0x021098C8, 10)
static bool Stage_customSpecialPlayerBump(Player* self, Player* other, fx32& selfCollisionPointX)
{
	bool marioOffender = Player::bumpOffender == Player::BumpOffender::Mario;
	Player* offender = marioOffender ? self : other;
	Player* victim = marioOffender ? other : self;

	if (offender->checkGroundpoundBump())
	{
		s32 direction = StageEntity::unitDirection[(selfCollisionPointX < 0) ^ marioOffender];
		Vec2 velocity(0xD00 * direction, 0x3000);
		victim->doPlayerBump(velocity, true);
		return true;
	}

	return false;
}

NTR_USED static bool Stage_blockPlayerCollision()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);
		if (player->currentPowerup == PowerupState::Mega || Stage_hasLevelFinished())
			return true;
	}
	return false;
}

asm(R"(
ncp_jump(0x021096EC, 10)
	BNE     0x021096F0
	MOV     R0, R9
	MOV     R1, R8
	BL      _ZL26Stage_blockPlayerCollisionv
	CMP     R0, #0
	BNE     0x021096F0
	B       0x02109704
)");

ncp_repl(0x020E3260, 10, "MOV R0, R4") // Fireballs pass through player
//ncp_repl(0x020E32C4, 10, "ADD SP, SP, #0x10; POP {R4-R6,PC}") // Player immune to fireballs
