#include "nsmb.h"

int repl_020A333C_ov_00() { return 0; } //Use singleplayer stage setup crap
int repl_0201FC30() { return 0; } //Setup some level stuff

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

int repl_020A3588_ov_00() { return 0; } //Force update singleplayer top counter values
int repl_020A3734_ov_00() { return 0; } //Force draw singleplayer top counter values
int repl_020A3578() { return 0; } //Draw Luigi's HUD with Mario's values (shared coins)

void repl_020BFE68_ov_00() { asm("MOV R2, #0"); } //Force draw singleplayer bottom counter values
int repl_020C03D8_ov_00() { return 0; } //Force update singleplayer bottom counter values
int repl_020BE898_ov_00() { return 0; } //Draw bottom screen powerups singleplayer way (BNCL Powerup)
int repl_020BE91C_ov_00() { return 0; } //Draw bottom screen powerups singleplayer way (PALETTE Powerup)
int repl_020BE9E4_ov_00() { return 0; } //Draw bottom screen powerups singleplayer way (BNCL Circle)
int repl_020C03F4_ov_00() { return 0; } //Display Mario's score instead of local player score

int repl_020BE5E8_ov_00() { return 212; } //MvsL progress bar uses singleplayer pixel scale
void repl_020BE60C_ov_00() { asm("MOV R8, #6"); } //MvsL progress bar uses singleplayer OAM y_shift
void repl_020BE64C_ov_00() { asm("LDR R1, =0x020CA104"); } //MvsL progress bar uses singleplayer OAM addresses
int repl_020BE658_ov_00() { return 7; } //MvsL progress bar uses singleplayer BNCL rectangle index
void repl_020BED88_ov_00() {} //Do not draw singleplayer player position indicators on progress bar
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
	OAMEntry** liveCounterForPlayer_1P = (OAMEntry**)0x20CA00C;

	OAM_UpdateDigits(liveCounterForPlayer_1P[0], entryTable_1P, GetLivesForPlayer(0), 2, 3);
	OAM_UpdateDigits(liveCounterForPlayer_1P[1], entryTable_1P, GetLivesForPlayer(1), 2, 3);
}

//Fix end level text
void repl_020FC830_ov_0A() { asm("MOV R1, #0"); }
void repl_020FC234_ov_0A() { asm("MOV R1, #0"); }
void nsub_020FBF60_ov_0A() {}

//Use singleplayer time out instead of battle end
int repl_020FF79C_ov_0A() { return 0; }

int repl_02020374() { return 0; } //Force get coins in singleplayer mode

void nsub_0209AAD0_ov_00() {} //Disable MvsL coin score
void repl_020D3350_ov_0A() {} //Disable MvsL coin score for coin actor

int repl_0209AC1C_ov_00() { return 1; } //Allow score incrementation
int repl_0209ABA8_ov_00() { return 1; } //Allow score incrementation from actors

void nsub_02020300() { asm("MOV R0, #0"); asm("BICS R2, R0, #1"); asm("B 0x02020304"); } //All score goes to Mario instead of local player
void repl_02020358() { asm("MOV R4, #0"); } //Share player coins (all coins go to Mario)
void nsub_020203E4() //When Mario gets 1-up, also give Luigi 1-up.
{
	asm("MOV     R1, #0"); //PlayerNumber 0
	asm("MOV     R0, #8");
	asm("BL      0x0209AB04"); //Increment1UpForPlayer
	asm("MOV     R1, #1"); //PlayerNumber 1
	asm("MOV     R0, #8");
	asm("BL      0x0209AB04"); //Increment1UpForPlayer
	asm("B       0x20203F0"); //Return to code
}

void nsub_020204F8() { asm("B 0x02020514"); } //Allow player lives to decrease
//int repl_0201E508() { return *(int*)0x02085AB8; } //Respawn at midway control
void repl_02119CB8_ov_0A() {} //Do not freeze timer on player death
//Do not allow player to respawn so we can control it ourselves
void repl_0212B334_ov_0B() { asm("MOV R0, R6"); asm("MOV R1, R4"); }
int repl_0212B338_ov_0B(int playerNo, int lives)
{
	if ((lives == 0 && GetLivesForPlayer(!playerNo) == 0) || GetPlayerDeathState(!playerNo))
	{
		ExitLevel(false);
		return 0; //Do not respawn
	}
	else if (lives == 0)
	{
		return 0;
	}
	return 1; //Respawn
}

void nsub_0211C470_ov_0A() { asm("B 0x0211C580"); } //Remove pipe entrance on respawn
void nsub_0201E504() { asm("MOV R0, R5"); asm("MOV R1, R4"); asm("B 0x0201E54C"); }
void repl_0201E54C(Vec3* entranceData, int playerNo)
{
	PlayerActor* oppositePlayer = GetPtrToPlayerActorByID(!playerNo);

	((u8**)0x0208B0A0)[playerNo][18] = oppositePlayer->info.ViewID;

	entranceData->x = oppositePlayer->actor.position.x;
	entranceData->y = oppositePlayer->actor.position.y;
	entranceData->z = 0;

	u32 x = entranceData->x >> 12;
	u32 y = (-entranceData->y) >> 12;
	bool IsSolid = (GetTileBehaviorAtPos(x - 16, y - 8) >> 16) & 1;
	if(!IsSolid)
		entranceData->x -= 1.5 * 0x10000;

	SpawnParticle(249, entranceData);
	SpawnParticle(250, entranceData);
}

//Only freeze timer and pause menu on toad houses
void nsub_0212B908_ov_0B(u8* player)
{
	if (*(int*)0x02085A18 == 8)
	{
		*(int*)0x020CA898 |= 0x40;
		*(int*)0x020CA880 |= 0x10;
		player[1968] = 1;
		player[454] |= 1;
	}
}

int repl_02162110_ov_36() { return *(int*)0x02085A7C; } //Midway point plays sound at local player position

int repl_0215F1BC_ov_36() { return 4; } //Load the MvsL BMG instead of the singleplayer one

void repl_020F4684_ov_0A() { asm("MOV R1, #0x8E"); } //Allow some 3D graphics to load

int repl_020A18A8_ov_00() { return 0; } //Allow level to be exited

int repl_020A26C4_ov_00() { return 0; } //Force the game to update some singleplayer counters instead

int repl_02152944_ov_36() { return *(int*)0x02085A50; } //Allow Luigi lives on stage intro scene
int repl_0215293C_ov_36() { return *(int*)0x02085A50; } //Allow Luigi head on stage intro scene

int repl_020C0958_ov_00() { return 0; } //Allow singleplayer bottom background setup