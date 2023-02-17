#include "nsmb.h"

//Custom Ending.
bool Start = false;
int FinalBossPeachTimer = 0;

void hook_02144024_ov_28()
{
	if(Start == false)
	{
		((u8*)GetSpawnedActor(115, 0, 0))[1384] = 2;
		Start = true;
	}
	
	if(isEventActive(21))
	{
		//Player Number crap for Credits.
		u8 *PlayerNumber = (u8*) (0x2085A7C);
		u8* MenuPlayerNumber = (u8*)(0x20887F0);
		*MenuPlayerNumber = 0;
		*PlayerNumber = 0;
		
		if(FinalBossPeachTimer == 0)
		{	
			Music_StartMusicNumber(19);
			((u8*)GetSpawnedActor(115, 0, 0))[1384] = 4;
			
			PlaySNDEffect(205, &GetPtrToPlayerActorByID(0)->actor.position);
			PlaySNDEffect(390, &GetPtrToPlayerActorByID(0)->actor.position);
		}
		
		if(FinalBossPeachTimer == 1)
		{
			((u8*)GetSpawnedActor(115, 0, 0))[1384] = 4;
		}
		
		if(FinalBossPeachTimer == 2)
		{
			((u8*)GetSpawnedActor(115, 0, 0))[1384] = 4;
		}
		
		FinalBossPeachTimer++;
		*(int*)0x020CA8D0 |= 0x8000; //Another bitmask
		
		//Mario
		int MarioXPoint = (41.2 * 65536);
		GetPtrToPlayerActorByID(0)->actor.position.x = 41.2 * 65536;
		GetPtrToPlayerActorByID(0)->actor.position.y = 15 * 65536 * -1;

		//Luigi
		int LuigiXPoint = (38.8 * 65536);
		GetPtrToPlayerActorByID(1)->actor.position.x = 38.8 * 65536;
		GetPtrToPlayerActorByID(1)->actor.position.y = 15 * 65536 * -1;
		
		for (int i = 0; i < GetPlayerCount(); i++)
		{
			PlayerActor_SetAnimation(GetPtrToPlayerActorByID(i), 0x87, 0, 0, 4096, 0);
			GetPtrToPlayerActorByID(i)->actor.rotation.y = 0;
			PlayerActor_freeze(GetPtrToPlayerActorByID(i), 1);
			GetPtrToPlayerActorByID(i)->P.jumpBitfield |= 0x1000000;
		}
	}
	
	//4. Exit Level.
	if(FinalBossPeachTimer > 1100)
	{
		Start = false;
		FinalBossPeachTimer = 0;
		ExitLevel(true);
	}
	
	//3. Kiss Part.
	if(FinalBossPeachTimer > 950)
	{
		FinalBossPeachTimer++;
		for (int i = 0; i < GetPlayerCount(); i++)
		{
			PlayerActor_SetAnimation(GetPtrToPlayerActorByID(0), 100, 0, 0, 7500, 0); //100
			PlayerActor_SetAnimation(GetPtrToPlayerActorByID(1), 0x8B, 0, 0, 3000, 0);
			GetPtrToPlayerActorByID(i)->actor.rotation.y = 0;
			PlayerActor_freeze(GetPtrToPlayerActorByID(i), 1);
			GetPtrToPlayerActorByID(i)->P.jumpBitfield |= 0x1000000;
		}
	}
	
		
	//2.Spin Part
	else if(FinalBossPeachTimer > 620)
	{
		FinalBossPeachTimer++;
		for (int i = 0; i < GetPlayerCount(); i++)
		{
			PlayerActor_SetAnimation(GetPtrToPlayerActorByID(0), 0x8B, 0, 0, 3500, 0); //100
			PlayerActor_SetAnimation(GetPtrToPlayerActorByID(1), 0x8B, 0, 0, 3500, 0);
			GetPtrToPlayerActorByID(i)->actor.rotation.y = 0;
			PlayerActor_freeze(GetPtrToPlayerActorByID(i), 1);
			GetPtrToPlayerActorByID(i)->P.jumpBitfield |= 0x1000000;
		}
	}
	
	//1.Blocks under Peach get destroyed.
	else if(FinalBossPeachTimer > 396)
	{
		FinalBossPeachTimer++;
		deactivateEvent(21);
		
		for (int i = 0; i < GetPlayerCount(); i++)
		{
			PlayerActor_SetAnimation(GetPtrToPlayerActorByID(i), 0x00, 0, 0, 4096, 0);
			GetPtrToPlayerActorByID(0)->actor.rotation.y = -5000;
			GetPtrToPlayerActorByID(1)->actor.rotation.y = 5000;
			PlayerActor_freeze(GetPtrToPlayerActorByID(i), 1);
			GetPtrToPlayerActorByID(i)->P.jumpBitfield |= 0x1000000;
		}
		
		((u8*)GetSpawnedActor(115, 0, 0))[1384] = 5;
		activateEvent(22);
	}
}