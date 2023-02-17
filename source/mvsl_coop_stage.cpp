#include "nsmb.h"

// ======================================= CAMERA FUNCTIONS =======================================

static int CameraForPlayerNo[2] = { 0 };
extern "C" {
	int GetCameraForPlayerNo(int playerNo) { return CameraForPlayerNo[playerNo]; };
	void SetCameraForPlayerNo(int playerNo, int focusPlayerNo) { CameraForPlayerNo[playerNo] = focusPlayerNo; };
}
void nsub_020201A4() { asm("MOV R4, R0"); asm("BL GetCameraForPlayerNo"); asm("B 0x20201A8"); }

void hook_020FF864_ov_0A(PlayerActor* player) { int playerNo = player->actor.playerNumber; SetCameraForPlayerNo(playerNo, playerNo); }

// ======================================= MISC =======================================

int repl_020AECA4_ov_00() { return 1; } //Disable background HDMA parallax

int repl_020BD820_ov_00() { return GetPlayerCount(); } //Bottom screen background draw
int repl_020BDA90_ov_00() { return GetPlayerCount(); } //Bottom screen background execute
int repl_020BDC1C_ov_00() { return GetPlayerCount(); } //Bottom screen background load

int repl_020A3578_ov_00() { return 0; } //Draw Luigi's HUD with Mario's values (shared coins)
int repl_020C03F4_ov_00() { return 0; } //Display Mario's score instead of local player score

int repl_020BE5E8_ov_00() { return 212; } //MvsL progress bar uses singleplayer pixel scale
void repl_020BE60C_ov_00() { asm("MOV R8, #6"); } //MvsL progress bar uses singleplayer OAM y_shift
void repl_020BE64C_ov_00() { asm("LDR R1, =0x020CA104"); } //MvsL progress bar uses singleplayer OAM addresses
int repl_020BE658_ov_00() { return 7; } //MvsL progress bar uses singleplayer BNCL rectangle index
void repl_020BED88_ov_00() {} //Do not draw singleplayer player position indicators on progress bar
bool repl_020BE5C4_ov_00(int playerNo) { return GetPtrToPlayerActorByID(playerNo) && GetPlayerDeathState(playerNo) != 2; } //Hide dead player
extern "C" {
	void MvsLDrawBottomScreenProgressPathIcons(int* stageScene, int x_shift, int y_shift);
	void DrawBottomScreenProgressPathIcons(int* stageScene);
}
void repl_020BF124_ov_00(int* stageScene) //Draw MvsL progress bar instead
{
	MvsLDrawBottomScreenProgressPathIcons(stageScene, 0, 0);
	DrawBottomScreenProgressPathIcons(stageScene);
}

//Draw bottom screen lives my way
void nsub_020BEC60_ov_00()
{
	OAMEntry** liveCounterForPlayer_1P = (OAMEntry * *)0x020CA00C;
	int x_shift = *(int*)0x020CC2C4;

	OAM_DrawHUDSubFromLoadedBNCL(6, liveCounterForPlayer_1P[0], 0, 0, 0, 0, 0, 0, 0, -x_shift - 64 - 4, 0);
	OAM_DrawHUDSubFromLoadedBNCL(6, liveCounterForPlayer_1P[1], 0, 0, 0, 0, 0, 0, 0, -x_shift + 4, 0);
}

//Update lives for both players
void nsub_020C041C_ov_00() { asm("B 0x020C0444"); }
void repl_020C0444_ov_00()
{
	u32* entryTable_1P = (u32*)0x216F554;
	OAMEntry** liveCounterForPlayer_1P = (OAMEntry * *)0x20CA00C;

	OAM_UpdateDigits(liveCounterForPlayer_1P[0], entryTable_1P, GetLivesForPlayer(0), 2, 3);
	OAM_UpdateDigits(liveCounterForPlayer_1P[1], entryTable_1P, GetLivesForPlayer(1), 2, 3);
}

void nsub_0209AAD0_ov_00() {} //Disable MvsL coin score
void repl_020D3350_ov_0A() {} //Disable MvsL coin score for coin actor

int repl_0209AC1C_ov_00() { return 1; } //Allow score incrementation
int repl_0209ABA8_ov_00() { return 1; } //Allow score incrementation from actors

void nsub_02020300() { asm("MOV R0, #0"); asm("BICS R2, R0, #1"); asm("B 0x02020304"); } //All score goes to Mario instead of local player
void repl_02020358() { asm("MOV R4, #0"); } //Share player coins (all coins go to Mario)
void repl_020203EC() //When Mario gets 1-up from coins, also give Luigi 1-up.
{
	for (int i = 0; i < GetPlayerCount(); i++)
		GiveScoreItemForPlayer(8, i);
}

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

//Fix desyncs on pause menu
u16* repl_020A20E8_ov_00(u8* stageScene) { asm("MOV R0, R5"); return &((u16*)0x2087648)[stageScene[25640]]; }
u8 repl_020A21A4_ov_00(u8* stageScene) { asm("MOV R0, R5"); return stageScene[25640]; }
u8 repl_020A22D8_ov_00(u8* stageScene) { return repl_020A21A4_ov_00(stageScene); }
//Disable options on pause menu
void repl_020A2230_ov_00() {
	if (GetPlayerCount() == 1)
		asm("BL 0x20C1F14");
}

// ======================================= MISC =======================================

//Fix some bottom screen locking crap that took our time (related to wavy fading transition) :(
void nsub_0201EB1C() { asm("B 0x0201EB40"); }
void repl_0201EB4C() {}

//Fix some bottom screen locking crap that took our time (related to wavy fading transition) :(
void nsub_0201EB1C() { asm("B 0x0201EB40"); }
void repl_0201EB4C() {}

//Disable baphs if player count is bigger than 1 (prevents desyncs)
void nsub_02012584()
{
	asm("BL GetPlayerCount");
	asm("CMP R0, #1");
	asm("BGT 0x0201258C");
	asm("LDR R0, =0x2088B94");
	asm("B 0x02012588");
}

void nsub_020FBF60_ov_0A() {} //Fix end of level for player that "lost the race"

int repl_021624C8_ov_36() { return *(int*)0x02085A7C; } //Midway point draws from local player
int repl_02162110_ov_36() { return *(int*)0x02085A7C; } //Midway point plays sound at local player position

void repl_0215ED54_ov_36() {} //Disable mega mushroom destruction counter
void repl_02157514_ov_36(){} //Disables coins spawn from mega mushroom groundpound.
void repl_02157534_ov_36(){} //Disables Goomba spawn from mega mushroom groundpound.

int repl_02152944_ov_36() { return *(int*)0x02085A50; } //Allow Luigi lives on stage intro scene
int repl_0215293C_ov_36() { return *(int*)0x02085A50; } //Allow Luigi head on stage intro scene

void repl_020FBD70_ov_0A() {} //Disables "Lose" music. (End Flag & Boss)

//Mega Mushroom enemy spawn desync fix
void repl_02157514_ov_36() {} //Disables Coins spawn from mega mushroom groundpound.
void repl_02157534_ov_36() {} //Disables Goomba spawn from mega mushroom groundpound.

// ======================================= PLAYER ACTOR =======================================

void repl_021096EC_ov_0A() {} //Disable Mario & Luigi Collision

//Mario Updater!
void hook_020201A0()
{
	///Death Function (0 Lives).
	s8 *MarioLives = (s8*) (0x208B364); // Mario's Lives
	s8 *LuigiLives = (s8*) (0x208B368); // Luigi's Lives
	
	PlayerActor* Mario = GetPtrToPlayerActorByID(0);
	PlayerActor* Luigi = GetPtrToPlayerActorByID(1);
	
	//If Mario has no lives.
	if(*MarioLives <= 0)
	{
		if(!GetPlayerDeathState(0))
		{
			Mario->P.DeathState = 0x21197FC;
		}
	}
	
	//If Luigi has no lives.
	if(*LuigiLives <= 0)
	{
		if(!GetPlayerDeathState(1))
		{
			Luigi->P.DeathState = 0x21197FC;
		}
	}
}



/*
//EndOfLevelFlag__onExecute(). This will cause players to shrink if too close to the flag!
void hook_0212FCAC_ov_0C()
{
	int *MegaMushroomTime = (int*) (0x208B334);
	int Buffer = (int) (0x208B334);
	
	//Remove mega if in range of flag.
	if(Buffer >= 1)
	{
		Music_StopSeq(03);
		*MegaMushroomTime = 0;
		int seqNo = Music_GetLevelSeqNo(0);
		Music_StartMusicNumber(seqNo);
	}
}
*/
