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