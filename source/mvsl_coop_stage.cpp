#include "nsmb.h"

int repl_020AECA4_ov_00() { return 1; } //Disable background scrolling parallax

int repl_020BD820_ov_00() { return GetPlayerCount(); } //Bottom screen background draw
int repl_020BDC1C_ov_00() { return GetPlayerCount(); } //Bottom screen background load

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
	for(int i = 0; i < GetPlayerCount(); i++)
		GiveScoreItemForPlayer(8, i);
}

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

//Pause menu
u16* repl_020A20E8_ov_00(u8* stageScene) { asm("MOV R0, R5"); return &((u16*)0x2087648)[stageScene[25640]]; }
u8 repl_020A21A4_ov_00(u8* stageScene) { asm("MOV R0, R5"); return stageScene[25640]; }
u8 repl_020A22D8_ov_00(u8* stageScene) { return repl_020A21A4_ov_00(stageScene); }
//Disable options on pause menu
void repl_020A2230_ov_00() {
	if (GetPlayerCount() == 1)
		asm("BL 0x20C1F14");
}

//Disable baphs if player count is bigger than 1 (prevents desyncs)
void repl_02012584()
{
	asm("BL GetPlayerCount");
	asm("CMP R0, #1");
	asm("BGT 0x0201258C");
	asm("LDR R0, =0x2088B94");
	asm("B 0x02012588");
}

int repl_02152944_ov_36() { return *(int*)0x02085A50; } //Allow Luigi lives on stage intro scene
int repl_0215293C_ov_36() { return *(int*)0x02085A50; } //Allow Luigi head on stage intro scene