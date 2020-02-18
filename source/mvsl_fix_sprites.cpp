#include "nsmb.h"

//Fix Volcano BG
void repl_020B6B6C(){asm("MOV     R1, #0");}

//Remove delete range for UnusedSpikeBass (256).
void repl_021728EC_ov_3A(){}

//Fix Rotating Barrel (246) Desync.
void repl_02186F50_ov_60(){asm("MOV     R1, #0");}

//Fix horizontal movement mushroom. (DeleteIfOutOfRange) (PROBABLY DOESN'T WORK)
void repl_0217FDDC_ov_5A(){}

//Fix Boo (DeleteIfOutOfRange) (PROBABLY DOESN'T WORK)
void repl_02175CA4_ov_47(){}

//Fix Dorie (DeleteIfOutOfRange) Doesn't work.
void repl_021474FC_ov_2F(){ asm("MOV     R0, #0"); }

//Fix rotating carry platform (BooHouse, DeleteIfOutOfRange)
void repl_0218EC4C_ov_76(){}

//Pipe Cannon (Turns it into normal pipe).
void repl_020F8470_ov_0A(){}

///Lava (234) & Poisoned Water (259)
//Water_executeState0 (Fixes Lava & Poisoned Water for Multiplayer).
void hook_02164838_ov_36()
{	
	//Only apply if Lava (234) is active.
	bool IsLavaActive = (EnemyActor*)GetSpawnedActor(279, 0, 0);
	if(IsLavaActive)
	{	
		//Kill Player if swimming.
		for (int i = 0; i < GetPlayerCount(); i++)
		{
			PlayerActor* Player = GetPtrToPlayerActorByID(i);
			if(Player->P.collBitfield & IN_LIQUID)
			{
				if(GetPlayerDeathState(i) == false)
				{
					Player->P.DeathState = 0x211997C;
				}
			}
		}
	}
	
	//Only apply if Poisoned Water (259) is active.
	bool IsPoisonWaterActive = (EnemyActor*)GetSpawnedActor(281, 0, 0);
	if(IsPoisonWaterActive)
	{	
		//Kill Player if swimming.
		for (int i = 0; i < GetPlayerCount(); i++)
		{
			PlayerActor* Player = GetPtrToPlayerActorByID(i);
			if(Player->P.collBitfield & IN_LIQUID)
			{
				if(GetPlayerDeathState(i) == false)
				{
					Player->P.DeathState = 0x21197FC;
				}
			}
		}
	}
}
void repl_02165534_ov_36(){asm("MOV R3, #1");} //Force Lava (234) to behave like Water.
void repl_021653EC_ov_36(){asm("MOV R3, #1");} //Force Poisoned Water (259) to behave like Water.



