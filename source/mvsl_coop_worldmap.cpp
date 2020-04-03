#include "nsmb.h"

static u8* PlayAsLuigi = (u8*)(0x2085A50);
static u8* MenuPlayerNumber = (u8*)(0x20887F0);

void repl_021578F0_ov_34() { asm("MOVEQ R1, #0"); } //Force MvsLMode = 0
void nsub_021535A0_ov_34() { SetPlayerCount(2); } //Change mvsl setup crap

void repl_020CF880_ov_08() {} //Delete worldmap disconnecter

int repl_02157A40_ov_34()
{
	*PlayAsLuigi = *MenuPlayerNumber; //Enable Luigi graphics in world map for Luigi console (also use to store original menu controller)

	LoadSaveAndSetCounters(0, 0, &saveData);
	if (((u16*)&saveData.lives)[1] == 0)
		SetLivesForPlayer(1, 5);

	asm("LDR R1, =0x02088BDC");
	asm("LDR R1, [R1, #0x20]"); //saveData.currentWorld
	return WORLDMAP_SCENE; //WORLDMAP_SCENE
}

//On Worldmap Scene Creation
void hook_020CF7D0_ov_08()
{
	*MenuPlayerNumber = 0; //Mario controls Luigi in world map
}

//Replace world load level system
void repl_020CEF84_ov_08(int a_SceneID, int a_MvsLMode, int a_World, int a_Level, int a_Area, int a_PlayerNumber, int a_SpawnBitmask, int a_P1Character, int a_P2Character, int a_StartingPowerup, int a_EntranceID, int a12, int a13, int a14, int a15, int a16, int a17)
{
	if (GetConsoleCount() == 1)
	{
		ChangeSceneToLevel(a_SceneID, a_MvsLMode, a_World, a_Level, a_Area, a_PlayerNumber, a_SpawnBitmask, a_P1Character, a_P2Character, a_StartingPowerup, a_EntranceID, a12, a13, a14, a15, a16, a17);
	}
	else
	{
		*MenuPlayerNumber = *PlayAsLuigi; //Restore menu player number

		int PowerupForPlayer[2];
		PowerupForPlayer[0] = GetPowerupForPlayer(0);
		PowerupForPlayer[1] = GetPowerupForPlayer(1);
		
		ChangeSceneToLevel(a_SceneID, a_MvsLMode, a_World, a_Level, 2, *MenuPlayerNumber, 3, 0, 1, 0, a_EntranceID, a12, a13, a14, a15, a16, a17);

		SetPowerupForPlayer(0, PowerupForPlayer[0]);
		SetPowerupForPlayer(1, PowerupForPlayer[1]);
	}
}

//Disable pause menu on worldmap
void repl_020CE944_ov_08() {
	if (GetConsoleCount() == 1)
		asm("BL 0x20C1F14");
}

//Fix top OAM powerup on worldmap
int repl_020CF184_ov_08() { return *PlayAsLuigi; }

//Fix player on worldmap
int repl_020CE2A4_ov_08() { return *PlayAsLuigi; }
int repl_020D5C18_ov_08() { return *PlayAsLuigi; }
int repl_020D5FC0_ov_08() { return *PlayAsLuigi; }
int repl_020D63C0_ov_08() { return GetPowerupForPlayer(*PlayAsLuigi); }
int repl_020D83D4_ov_08() { return *PlayAsLuigi; }
int repl_020D8D30_ov_08() { return *PlayAsLuigi; }
int repl_020D8D44_ov_08() { return *PlayAsLuigi; }