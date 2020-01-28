#include "nsmb.h"

// MAIN CAMERA PUSH =============================

//Allow camera to be pushed for all players
void nsub_020ACF50_ov_00(int* LevelActor, int Arg2, int Case)
{
	int Var1 = Arg2 << 12;
	for (int PlayerNumber = 0; PlayerNumber < GetPlayerCount(); PlayerNumber++)
	{
		switch (Case)
		{
		case 0:
			if (Var1 > ((int*)0x020CAE64)[2 * PlayerNumber])
				Var1 = ((int*)0x020CAE64)[2 * PlayerNumber];
			LevelActor[5 * PlayerNumber + 0x2A40] = Var1;
			break;
		case 1:
			if (Var1 < ((int*)0x020CAE60)[2 * PlayerNumber])
				Var1 = ((int*)0x020CAE60)[2 * PlayerNumber];
			LevelActor[5 * PlayerNumber + 0x2A3F] = Var1;
			break;
		case 2:
			if (Var1 > ((int*)0x020CAE74)[2 * PlayerNumber])
				Var1 = ((int*)0x020CAE74)[2 * PlayerNumber];
			LevelActor[5 * PlayerNumber + 0x2A3E] = Var1;
			break;
		case 3:
			if (Var1 < ((int*)0x020CAE70)[2 * PlayerNumber])
				Var1 = ((int*)0x020CAE70)[2 * PlayerNumber];
			LevelActor[5 * PlayerNumber + 0x2A3D] = Var1;
			break;
		default:
			return;
		}
	}
}

// BOWSER JR =============================

void repl_0213D0BC_ov_1C() //Victory freeze on ground touch
{
	for (int i = 0; i < GetPlayerCount(); i++)
		GetPtrToPlayerActorByID(i)->P.jumpBitfield |= 0x1000000;
}
void repl_0213F49C_ov_1C() //Freeze the player while falling
{
	for (int i = 0; i < GetPlayerCount(); i++)
	{
		PlayerActor* player = GetPtrToPlayerActorByID(i);
		player->P.miscActionsBitfield |= 0x80;
		player->actor.velocity.x = 0;
	}
}
void repl_0213F4A0_ov_1C() {} //Supports function above ^
void repl_0213F4B4_ov_1C() //Freeze player when touching the ground after falling
{
	for (int i = 0; i < GetPlayerCount(); i++)
		PlayerActor_freeze(GetPtrToPlayerActorByID(i), 0);
}
void repl_0213F550_ov_1C() //Unfreeze when battle is ready to start
{
	for (int i = 0; i < GetPlayerCount(); i++)
		PlayerActor_unfreeze(GetPtrToPlayerActorByID(i));
}

// BOWSER =============================

//Allow unfreeze for all players
void repl_0213695C_ov_0D()
{
	for (int i = 0; i < GetPlayerCount(); i++)
		PlayerActor_unfreeze(GetPtrToPlayerActorByID(i));
}

//Fix fireball tracking
void nsub_02138D7C_ov_0D() { asm("MOV R3, #0"); asm("B 0x02138D80"); }

// MUMMY POKEY =============================

void repl_02131EDC_ov_10()
{
	for (int i = 0; i < GetPlayerCount(); i++)
		PlayerActor_unfreeze(GetPtrToPlayerActorByID(i));
}

// FINAL BOSS CONTROLLER =============================

void repl_0213BFA8_ov_1C() { asm("LDR R1, =0x1000001"); asm("BX LR"); } //Bowser Jr. camera spawn fix
void nsub_0214827C_ov_2B() { asm("MOV R1, R4"); asm("B 0x02148338"); } //Set arguments for function below
void repl_02148338_ov_2B(PlayerActor* wall_player, EnemyActor* controller)
{
	static u8 Index_2P = 0;

	wall_player->P.miscActionsBitfield |= 0x80;
	wall_player->actor.velocity.x = 0;
	if (wall_player->actor.collisionBitfield & 0x1F40)
	{
		if (GetPlayerCount() == 1)
			goto freezeAll;
		PlayerActor_freeze(wall_player, 0);
		Index_2P = 1;
	}

	if (Index_2P > 0 && Index_2P < 255)
	{
		if (Index_2P <= 16)
		{
			setBrightness(3, -Index_2P);
		}
		else if (Index_2P == 17)
		{
		freezeAll:
			for (int i = 0; i < GetPlayerCount(); i++)
			{
				PlayerActor* player = GetPtrToPlayerActorByID(i);
				player->P.miscActionsBitfield |= 0x80;
				player->actor.velocity.x = 0;
				player->P.unk784 |= 1;
				PlayerActor_freeze(player, 1);

				if (GetPlayerCount() == 1)
					goto setCutsceneVars;

				player->actor.position = controller->actor.position;
				player->actor.position.x -= (19 + (i * 3)) * 0x8000;
			}
		}
		else if (Index_2P > 77 && Index_2P <= 93)
		{
			setBrightness(3, (Index_2P - 77) - 16);
		}
		else if (Index_2P == 153)
		{
		setCutsceneVars:

			Music_StopSeq(30);

			*(int*)0x020CA880 |= 1; //Pause menu bitmask maybe
			*(int*)0x020CA898 |= 0x10; //Time bitmask
			*(int*)0x020CA8D0 |= 0x8000; //Another bitmask

			*((u16*)controller + 1377) = 1;

			if (GetPlayerCount() == 1)
				return;
		}
		Index_2P++;
	}
}