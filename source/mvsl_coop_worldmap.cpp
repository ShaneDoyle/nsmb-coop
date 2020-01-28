#include "nsmb.h"

static u8* PlayAsLuigi = (u8*)(0x2085A50);
static u8* MenuPlayerNumber = (u8*)(0x20887F0);

void repl_020CF880_ov_08() {} //Delete worldmap disconnecter

int repl_02157A40_ov_34()
{
	*PlayAsLuigi = *MenuPlayerNumber; //Enable Luigi graphics in world map for Luigi console (also use to store original menu playernumber)

	LoadSaveAndSetCounters(0, 0, &saveData); //Load save file 1

	asm("LDR R1, =0x02088BDC");
	asm("LDR R1, [R1, #0x20]"); //saveData.currentWorld
	return WORLDMAP_SCENE; //WORLDMAP_SCENE
}

//On Worldmap Scene Creation
void hook_020CF7D0_ov_08()
{
	//Fix player palettes
	for(int i = 1873 - 131; i <= 1897 - 131; i++)
		nFS_UnloadFileByIDFromCache(i);

	*MenuPlayerNumber = 0; //Mario controls Luigi in world map
}

//Replace world load level system
void repl_020CEF84_ov_08(int a_SceneID, int a_MvsLMode, int a_World, int a_Level, int a_Area, int a_PlayerNumber, int a_SpawnBitmask, int a_P1Character, int a_P2Character, int a_StartingPowerup, int a_EntranceID, int a12, int a13, int a14, int a15, int a16, int a17)
{
	int* MvsLMode = (int*)0x2085A84;
	if (*MvsLMode == 0)
	{
		ChangeSceneToLevel(a_SceneID, a_MvsLMode, a_World, a_Level, a_Area, a_PlayerNumber, a_SpawnBitmask, a_P1Character, a_P2Character, a_StartingPowerup, a_EntranceID, a12, a13, a14, a15, a16, a17);
	}
	else
	{
		*MenuPlayerNumber = *PlayAsLuigi; //Restore menu player number

		int PowerupForPlayer[2];
		PowerupForPlayer[0] = GetPowerupForPlayer(0);
		PowerupForPlayer[1] = GetPowerupForPlayer(1);

		ChangeSceneToLevel(a_SceneID, *MvsLMode, a_World, a_Level, a_Area, *MenuPlayerNumber, 3, 0, 1, 0, 255, 1, 1, 255, 0, 0, -1);

		SetPowerupForPlayer(0, PowerupForPlayer[0]);
		SetPowerupForPlayer(1, PowerupForPlayer[1]);
	}
}

//Fix player on worldmap
int repl_020CE2A4_ov_08() { return *PlayAsLuigi; }
int repl_020D5C18_ov_08() { return *PlayAsLuigi; }
int repl_020D5FC0_ov_08() { return *PlayAsLuigi; }
int repl_020D63C0_ov_08() { return GetPowerupForPlayer(*PlayAsLuigi); }
int repl_020D83D4_ov_08() { return *PlayAsLuigi; }
int repl_020D8D30_ov_08() { return *PlayAsLuigi; }
int repl_020D8D44_ov_08() { return *PlayAsLuigi; }

//Do not spawn particles in worldmap
void nsub_02022B74()
{
	asm("LDR     R0, =0x0203BD34"); //R0 = scene ID pointer
	asm("LDR     R0, [R0]"); //R0 = R0 pointer value
	asm("CMP     R0, #9"); //Compare R0 to scene 9
	asm("ADDEQ   SP, SP, #0x10");
	asm("LDMEQFD SP!, {R4,PC}");
	asm("MOV     R0, R4");
	asm("STR     R2, [SP]"); //Keep replaced instruction
	asm("B       0x02022B78"); //Return to code
}

//Do not spawn and end particles in worldmap
void nsub_020228A0()
{
	asm("LDR     R0, =0x0203BD34"); //R0 = scene ID pointer
	asm("LDR     R0, [R0]"); //R0 = R0 pointer value
	asm("CMP     R0, #9"); //Compare R0 to scene 9
	asm("ADDEQ   SP, SP, #0x1C");
	asm("LDMEQFD SP!, {R4-R9,PC}");
	asm("MOV     R0, R4");
	asm("STR     R5, [SP]"); //Keep replaced instruction
	asm("B       0x020228A4"); //Return to code
}