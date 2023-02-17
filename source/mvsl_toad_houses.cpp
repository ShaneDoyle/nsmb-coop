#include "nsmb.h"

void repl_0218CD60_ov_7B() //mushroom houses (freeze)
{
	for(int i = 0; i < GetPlayerCount(); i++)
		PlayerActor_freeze(GetPtrToPlayerActorByID(i), 0);
}

void repl_0218CE94_ov_7B() //mushroom houses (unfreeze)
{
	for (int i = 0; i < GetPlayerCount(); i++)
		PlayerActor_unfreeze(GetPtrToPlayerActorByID(i));
}

void repl_0218D1EC_ov_7B() //1up houses (freeze)
{
	for (int i = 0; i < GetPlayerCount(); i++)
		PlayerActor_freeze(GetPtrToPlayerActorByID(i), 0);
}

void repl_0218D480_ov_7B() //1up houses (unfreeze)
{
	for (int i = 0; i < GetPlayerCount(); i++)
		PlayerActor_unfreeze(GetPtrToPlayerActorByID(i));
}

void repl_0218CF20_ov_7B()
{	
	for (int i = 0; i < GetPlayerCount(); i++)
		GetPtrToPlayerActorByID(i)->P.physicsStateBitfield &= 0xDFFFFFFF;
}

void repl_0218E630_ov_7B()
{
	for (int i = 0; i < GetPlayerCount(); i++)
		GetPtrToPlayerActorByID(i)->P.physicsStateBitfield |= 0x20000000;
}

//Toadsworth__onExecute
void hook_0218C83C_ov_7B()
{
	//Temp hardcode for toad house.
	u32 *TilesetNumber = (u32*) (0x20CACBC); //Tileset
	
	//For Toad Houses
	if(*TilesetNumber == 42 || *TilesetNumber == 43)
	{
		u8 *MarioStorageItem = (u8*) (0x208B32C); //Mario's Item.
		u8 *LuigiStorageItem = (u8*) (0x208B32D); //Luigi's Item.
		
		//Player Number crap for Credits.
		u8 *PlayerNumber = (u8*) (0x2085A7C);
		*PlayerNumber = 0;
		
		*LuigiStorageItem = *MarioStorageItem;
	}
	else if(*TilesetNumber == 44)
	{
		u8 *MenuPlayerNumber = (u8*)(0x20887F0);
		*MenuPlayerNumber = 0;
	}
}