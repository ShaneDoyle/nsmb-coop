#include "nsmb.h"

//Fix Volcano BG
void repl_020B6B6C() { asm("MOV R1, #2"); }

//Remove delete range for UnusedSpikeBass (256).
void repl_021728EC_ov_3A() {}

//Fix Rotating Barrel (246) Desync.
void repl_02186F50_ov_60() { asm("MOV R1, #0"); }

//Fix horizontal movement mushroom. (DeleteIfOutOfRange) (PROBABLY DOESN'T WORK)
void repl_0217FDDC_ov_5A() {}

//Fix Boo (DeleteIfOutOfRange) (PROBABLY DOESN'T WORK)
void repl_02175CA4_ov_47() {}

//Fix Dorie (DeleteIfOutOfRange) Doesn't work.
void repl_021474FC_ov_2F() { asm("MOV R0, #0"); }

//Fix rotating carry platform (BooHouse, DeleteIfOutOfRange)
void repl_0218EC4C_ov_76() {}

// Fix pipe cannon desync.
void nsub_020F8230_ov_0A() { asm("B 0x020F823C"); }

//Red Coin Ring (Both players get items)
void nsub_0215410C_ov_36(Actor* red_ring)
{
	for (int i = 0; i < GetPlayerCount(); i++)
	{
		PlayerActor* player = GetPtrToPlayerActorByID(i);
		int playerNo = player->actor.playerNumber;

		Vec3 item_pos = player->actor.position.x & ((int*)0x02085AA4)[i];
		item_pos.y = -0x2000 - *(int*)0x020CAD94;

		Powerup old_powerup = player->P.powerupStateOld;
		int new_powerup = 0;
		if (old_powerup && old_powerup != MEGA)
		{
			if (old_powerup == SUPER)
				new_powerup = SUPER;
			else
				new_powerup = FIRE;
		}

		if (new_powerup == FIRE)
			if(playerNo == *(int*)0x02085A7C)
				PlaySNDEffect(0x17E, &red_ring->position);

		CreateActor(31, ((int*)0x216BFB8)[new_powerup] | ((playerNo & 1) << 16), &item_pos, 0, 0, 0);
	}
}

///Lava (234) & Poisoned Water (259)
void repl_020BBE88_ov_00() {} //Prevent liquid type change on respawn

//Lava
void repl_021654F8_ov_36() { asm("MOV R1, #0"); } //Force to Mario
void repl_0216550C_ov_36() { asm("MOV R0, #0"); } //Force to Mario
void hook_02165540_ov_36() //Luigi also gets his liquid type changed
{
	u8* liquidType = (u8*)0x20CACE0;
	liquidType[1] = liquidType[0];
}
//Poison
void repl_021653B4_ov_36() { asm("MOV R1, #0"); } //Force to Mario
void repl_021653C8_ov_36() { asm("MOV R0, #0"); } //Force to Mario
void hook_021653F8_ov_36() { hook_02165540_ov_36(); } //Luigi also gets his liquid type changed