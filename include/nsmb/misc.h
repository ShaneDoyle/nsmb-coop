#pragma once

#include <nds/ndstypes.h>
#include <nds/arm9/video.h>
#include "nsmb/vector.h"

#define HEXRGB15(hex) RGB15(hex & 0xFF0000, hex & 0x00FF00, hex & 0x0000FF);

#define IsDSiMode ((*((u8*)0x4004000)) & 1)

#define nkeysHeld ((u16*)0x02087650)
#define nkeysDown ((u16*)0x02087652)
#define nkeysUp ((u16*)0x02087640)
#define dashWithX ((u32*)0x02085AD4)

typedef enum Scene {
	BOOT_SCENE,
	CONNECT_SCENE,
	DEBUG_SCENE,
	STAGE_SCENE,
	MAIN_MENU_SCENE,
	MvsL_SETUP_SCENE,
	MvsL_MAIN_MENU_SCENE,
	SAVE_SCENE,
	WORLD_SCENE,
	WORLDMAP_SCENE,
	MvsL_RESULTS_SCENE,
	CORRUPTED_FILE_SCENE,
	ENDING_SCENE,
	STAGE_INTRO_SCENE,
	GAMEOVER_SCENE,
	MvsL_STAGE_INTRO_SCENE,
	SOUNDTEST_SCENE,
	KEY_SCENE,
	MINIGAMES_SCENE = 326,
} Scene;

typedef struct SaveData
{
	u32 header; //0
	u32 field_04; //4
	u32 Completion; //8
	u32 lives; //12
	u32 coins; //16
	u32 field_14; //24
	u32 starCoinsInSave; //32
	u32 spentStarCoins; //40
	u32 currentWorld; //42
	u32 field_24;
	u32 currentWorldMapNode;
	u32 field_2C;
	u32 currentPowerup;
	u32 score;
	u32 currentBottomScreenBackground;
	u32 field_3C;
	u32 field_40;
	u32 field_44;
	u32 field_48;
	u32 field_4C;
	u32 field_50;
	u32 field_54;
	u32 field_58;
	u8 inventoryItem;
	u8 field_5D;
	u8 field_5E;
	u8 field_5F;
	u16 worlds[8];
	u8 starCoins[200];
	u8 levels[240];
	u8 unk[32];
} SaveData;

extern SaveData saveData;

#ifdef __cplusplus
extern "C" {
#endif

	// Fades to another scene.
	// WARNING: Do not call if current scene is stage scene (level), use NSMB_ChangeToSceneFromLevel instead.
	void ChangeToScene(u16 SceneID, u32 SceneParameters);

	// Fades to another scene from the level (stage scene).
	void ChangeToSceneFromLevel(u16 SceneID, u32 SceneParameters);

	// Fades the scene to a level (SceneID: 13 = Singleplayer, 15 = Mario Vs Luigi)
	void ChangeSceneToLevel(u16 SceneID, u32 MvsLMode, u32 World, u32 Level, u8 Area, u8 PlayerNumber, u8 SpawnBitmask, u8 P1Character, u8 P2Character, u8 StartingPowerup, u8 EntranceID, u8 a12, u32 a13, u32 a14, u32 a15, u32 a16, u32 a17);

	// Leaves the level and goes to the worldmap or the mvsl menu
	void ExitLevel(bool Completed);

	// Writes saveData to the the backup file
	void SaveSave(int fileNo, SaveData* saveData);

	/* Reads a backup file to saveData and sets the counters, returns 1 if load succeeded
		if(lives != 0)
		saveData.lives = lives;
		saveData.currentPowerup = 0;*/
	int LoadSaveAndSetCounters(int fileNo, int lives, SaveData* saveData);

	// Crashes the game on purpose
	void OS_Panic(const char* fmt, ...);

	// Gets the file ID for a BMG
	int GetFileIDForBMG(int bmgNo);

	// Waits for video blanking
	void WaitForVBlank();

	// Flushes DC Range or All
	void DC_FlushRangeOrAll(const void* startAddr, u32 nBytes);

	void InitEntranceData(Vec3* entranceData, u32 entranceId, u32 playerNo);

	// Spawns a particle at a position
	void SpawnParticle(int particleId, Vec3* pos);

	// Gets the tile behavior at the position
	u32 GetTileBehaviorAtPos(int x, int y);

	// Gets if the player starts from midway or not
	bool GetStartingFromMidway();

	// 8 = 1-Up
	void GiveScoreItemForPlayer(int item, int playerNo);

	// Returns how many consoles are connected
	int GetConsoleCount();

	// Returns a random number
	int RNG();

#ifdef __cplusplus
}
#endif