#include "nsmb/game.h"
#include "nsmb/player.h"
#include "nsmb/system/misc.h"
#include "nsmb/stage/entity.h"

NTR_USED static u32 sTempVar;

// ======================================= GETTERS =======================================

static bool Stage_getLocalPlayerID() { return Game::localPlayerID; }
static bool Stage_getLuigiMode() { return Game::luigiMode; }

/*
// ======================================= CAMERA FUNCTIONS =======================================

static int CameraForPlayerNo[2] = { 0 };
extern "C" {
	int GetCameraForPlayerNo(int playerNo) { return CameraForPlayerNo[playerNo]; };
	void SetCameraForPlayerNo(int playerNo, int focusPlayerNo) { CameraForPlayerNo[playerNo] = focusPlayerNo; };
}
void nsub_020201A4() { asm("MOV R4, R0"); asm("BL GetCameraForPlayerNo"); asm("B 0x20201A8"); }

void hook_020FF864_ov_0A(PlayerActor* player) { int playerNo = player->actor.playerNumber; SetCameraForPlayerNo(playerNo, playerNo); }
*/
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
	return Game::getPlayer(playerID)/*TODO: && !sPlayerSpectating[playerID]*/;
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

/*
// Draw bottom screen lives my way
void nsub_020BEC60_ov_00()
{
	GXOamAttr** liveCounterForPlayer_1P = (GXOamAttr**)0x020CA00C;
	int x_shift = *rcast<int*>(0x020CC2C4);

	Game::drawBNCLSpriteSub(6, liveCounterForPlayer_1P[0], OAM::Flags::None, 0, 0, 0, 0, 0, OAM::Settings::None, -x_shift - 64 - 4, 0);
	Game::drawBNCLSpriteSub(6, liveCounterForPlayer_1P[1], OAM::Flags::None, 0, 0, 0, 0, 0, OAM::Settings::None, -x_shift + 4, 0);
}

// Update lives for both players
void nsub_020C041C_ov_00() { asm("B 0x020C0444"); }
void repl_020C0444_ov_00()
{
	u32* entryTable_1P = (u32*)0x0216F554;
	GXOamAttr** liveCounterForPlayer_1P = (GXOamAttr**)0x020CA00C;

	OAM_UpdateDigits(liveCounterForPlayer_1P[0], entryTable_1P, GetLivesForPlayer(0), 2, 3);
	OAM_UpdateDigits(liveCounterForPlayer_1P[1], entryTable_1P, GetLivesForPlayer(1), 2, 3);
}
*/
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
/*
// ======================================= ENTRANCE POSITIONING =======================================

//Force Luigi to spawn in the same entrance as Mario
void nsub_0215E5A4_ov_36()
{
	asm("LDR     R5, =0x020CA8F4");
	asm("LDRB    R5, [R5,#8]");
	asm("STRB    R5, [R1,#9]");
	asm("B       0x0215E5A8");
}
void nsub_0215EFF0_ov_36()
{
	asm("LDR     R2, =0x020CA8F4");
	asm("LDRB    R2, [R2,#8]");
	asm("STRB    R2, [R1,#9]");
	asm("B       0x0215EFF4");
}

//Player positioning on multiplayer entrance spawn
//entranceVecs[playerNumber]
void hook_0215E920()
{
	u8 entranceId = ((u8*)0x020CA8F4)[8];
	Entrance* entrance = GetPtrToEntranceById(entranceId, entranceId);
	Vec3* entranceVecs = (Vec3*)0x020CA928;

	switch (entrance->type)
	{
	//Pipe
	case 3:
	case 4:
	case 5:
	case 6:
		entranceVecs[1].x += 16 << 12;
		break;
	//Climbing Vine
	case 21:
		entranceVecs[0].x += 7 << 12;
		entranceVecs[1].x += 9 << 12;
		break;
	//Any other entrance
	default:
		entranceVecs[0].x += 8 << 12;
		entranceVecs[1].x += 8 << 12;
		break;
	}
}

// Center vine head
void repl_0211C218_ov_0A() { asm("MOV R0, R4"); }
void repl_0211C21C_ov_0A(PlayerActor* player)
{
	if (GetPlayerCount() == 1)
	{
		SpawnGrowingEntranceVine(&player->actor.position);
	}
	else if (player->actor.playerNumber == 0)
	{
		Vec3 pos = player->actor.position;
		pos.x -= 7 << 12;
		SpawnGrowingEntranceVine(&pos);
	}
}

// ======================================= RESPAWN =======================================

static u8 player_wasDead[2] = { false };

int repl_021041F4_ov_0A() { return GetPlayerCount() != 1; }
int repl_0212B318_ov_0B() { return GetPlayerCount() != 1; }
void repl_02119CB8_ov_0A() {} //Do not freeze timer on player death
void repl_0212B334_ov_0B() { asm("MOV R0, R6"); asm("MOV R1, R4"); } //Do not allow player to respawn so we can control it ourselves

void SetupRespawnLocationForPlayer(int playerNo)
{
	Entrance* destEntrance = ((Entrance**)0x0208B0A0)[!playerNo]; //Opposite player entrance
	u8* SwapScreenOnRespawnForPlayer = (u8*)0x0208B08C;
	SwapScreenOnRespawnForPlayer[playerNo] = destEntrance->settings & 1;

	player_wasDead[playerNo] = true;

	GetPtrToPlayerActorByID(playerNo)->actor.isVisible = false;
	PlayerActor* oppositePlayer = GetPtrToPlayerActorByID(!playerNo);
	fx32 x = oppositePlayer->actor.position.x;
	fx32 y = oppositePlayer->actor.position.y;
	SetRespawnPositionForPlayer(playerNo, x, y);
}

//New DecreaseLivesForPlayer that decreases immediately.
extern "C"
void DecLivesForPlayer_hook(int playerNo)
{
	if(GetLivesForPlayer(playerNo) > 0)
	{
		DecreaseLivesForPlayer(playerNo);
	}
}

void nsub_02119CA0_ov_0A()
{
  asm("LDRSB   R4, [R0,#0x1E]"); //Keep replaced instruction (player number get)
  asm("MOV     R0, R4"); //Pass playerNo as arg
  asm("BL      DecLivesForPlayer_hook"); //Run my custom function
  asm("LDR     R2, =0x020CA898"); //Make sure it doesn't get replaced by the hook
  asm("B       0x02119CA4"); //Return to code
}


//Remove original DecreaseLivesForPlayer and end level instead.
void repl_0212B2DC_ov_0B()
{
	if(GetPlayerDeathState(0))
	{
		if(GetPlayerDeathState(1))
		{
			ExitLevel(false);
		}
	}
}

//Respawning.
bool repl_0212B338_ov_0B(int playerNo, int lives)
{
	//Don't respawn.
	if ((lives == 0 && GetLivesForPlayer(!playerNo) == 0) || GetPlayerDeathState(!playerNo))
	{
		//ExitLevel(false);
		return false; //Do not respawn
	}
	//Disable for certain bosses, including Bowser JR. (All bosses except for Monty Tank). Also, disable for falling rock level.
	else if(SpriteSet1 == 5 || SpriteSet1 == 7 ||  SpriteSet16 == 2 || SpriteSet16 == 3 || SpriteSet16 == 4 || SpriteSet16 == 5 || SpriteSet16 == 6 || SpriteSet16 == 7)
	{
		return false;
	}
	//Respawn.
	SetupRespawnLocationForPlayer(playerNo);
	return true;
}

extern "C"
void PlayerActor_spectateLoop(PlayerActor* player, int playerNo)
{
	PlayerActor* oppositePlayer = GetPtrToPlayerActorByID(!playerNo);
	
	//Check for things we don't want to spawn again for. 
	bool AutoScroller = (EnemyActor*)GetSpawnedActor(218, 0, 0);
	
	//Check if player is allowed to respawn or not.
	if (GetLivesForPlayer(playerNo) != 0 && player->P.ButtonsPressed & KEY_A && GetPlayerDeathState(!playerNo) == 0 && SpriteSet16 != 8 && !AutoScroller)
	{
		player->P.cases = 1;

		player->actor.position.x = oppositePlayer->actor.position.x - 0x10000;
		player->actor.position.y = oppositePlayer->actor.position.y;

		player->actor.isVisible = true;

		player_wasDead[playerNo] = false;

		PlayerActor_setRespawnedState(player);
		SetRespawnStateForPlayer(playerNo, 3);

		int localPlayerNo = *(int*)0x02085A7C;
		if (playerNo == localPlayerNo)
		{
			int seqNo = Music_GetLevelSeqNo(playerNo);
			Music_StartMusicNumber(seqNo);
		}

		SpawnParticle(249, &player->actor.position);
		SpawnParticle(250, &player->actor.position);

		SetPlayerDeathState(playerNo, 0);
		SetCameraForPlayerNo(playerNo, playerNo);
	}
	else
	{
		Entrance* destEntrance = ((Entrance**)0x0208B0A0)[!playerNo]; //Opposite player entrance
		if (player->info.ViewID != destEntrance->view)
		{
			u8* SwapScreenOnRespawnForPlayer = (u8*)0x0208B08C;

			player->P.enteringAnEntrance = 2; //Force player to wait for the other
			SwapScreenOnRespawnForPlayer[playerNo] = destEntrance->settings & 1;
			fx32 x = (destEntrance->x) << 12;
			fx32 y = (-destEntrance->y) << 12;
			SetRespawnPositionForPlayer(playerNo, x, y);
			PlayerActor_setEntranceState(player, 0x0211870C, 0); //Call respawn system (Forces entrance reload)
		}
	}
}
//Setup hook for function above
void nsub_0211C470_ov_0A()
{
	asm("MOV     R0, R4");
	asm("LDRB    R1, [R4, #0x11E]");
	asm("BL      PlayerActor_spectateLoop");
	asm("B       0x0211C4EC");
}

//Set camera for player during respawn fade
void nsub_0201E558()
{
	asm("MOV     R0, R4"); //Move R0 to playerNo
	asm("MOV     R1, R0");
	asm("CMP     R1, #0"); //Move R1 to !playerNo
	asm("MOVEQ   R1, #1");
	asm("MOVNE   R1, #0");
	asm("BL      SetCameraForPlayerNo"); //Change camera
	asm("MOV     R0, R4"); //Move R0 to playerNo
	asm("MOV     R1, #2"); //Move R1 to 2
	asm("BL      SetPlayerDeathState"); //Set player as spectating
	asm("MOV     R0, #0xC"); //Keep replaced instruction
	asm("B       0x0201E55C"); //Return to code
}

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
}

// ======================================= PAUSE =======================================

// Fix desyncs on pause menu
u16* repl_020A20E8_ov_00(u8* stageScene) { asm("MOV R0, R5"); return &((u16*)0x2087648)[stageScene[25640]]; }
u8 repl_020A21A4_ov_00(u8* stageScene) { asm("MOV R0, R5"); return stageScene[25640]; }
u8 repl_020A22D8_ov_00(u8* stageScene) { return repl_020A21A4_ov_00(stageScene); }
//Disable options on pause menu
void repl_020A2230_ov_00() {
	if (GetPlayerCount() == 1)
		asm("BL 0x20C1F14");
}

*/
// ======================================= MISC =======================================

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

// ======================================= PLAYER ACTOR =======================================

//void repl_021096EC_ov_0A() {} //Disable Mario & Luigi Collision
