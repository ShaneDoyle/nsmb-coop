#include "nsmb.h"

//============================= Main Camera Push =============================

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

//============================= Bowser Jr. =============================

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

//============================= World 1: Bowser =============================

//Unfreeze both players
void repl_0213695C_ov_0D()
{
	for (int i = 0; i < GetPlayerCount(); i++)
		PlayerActor_unfreeze(GetPtrToPlayerActorByID(i));
}

//Fix fireball tracking
void repl_02138D7C_ov_0D() { asm("MOV R3, #0"); }

//Disables "StageZoom" for BowserBattleSwitch.
void repl_0213A7A4_ov_0D()
{
	for (int i = 0; i < GetPlayerCount(); i++)
		GetPtrToPlayerActorByID(i)->P.jumpBitfield |= 0x1000000;
}

//Remove Freeze at BowserBattleSwitch.
void repl_0213AF54_ov_0D() {}

//Bowser level exit
//void hook_0212FCAC_ov_0D() { ExitLevel(true); }

//============================= World 2: Mummy Pokey =============================

//Unfreeze both players
void repl_02131EDC_ov_10()
{
	for (int i = 0; i < GetPlayerCount(); i++)
		PlayerActor_unfreeze(GetPtrToPlayerActorByID(i));
}

//Disable ground-pound functionality for Mummy Pokey. (Removes weird de-sync issue, not finished!)
void nsub_02132960_ov_10() {}

//============================= World 3: Cheepskipper =============================
//On delete.
/*
void hook_0212FA18_ov_12()
{
	
}
*/

//============================= World 4: Mega Goomba =============================

//Unfreeze both players
void repl_0213137C_ov_0E()
{
	for (int i = 0; i < GetPlayerCount(); i++)
		PlayerActor_unfreeze(GetPtrToPlayerActorByID(i));
}


//============================= World 5: Petey Piranha =============================

//On create.
/*
void hook_0212FB00_ov_0F()
{

}
*/

//Unfreeze both players
void repl_02132A58_ov_0F()
{
	for (int i = 0; i < GetPlayerCount(); i++)
		PlayerActor_unfreeze(GetPtrToPlayerActorByID(i));
}
void repl_02132BE8_ov_0F()
{
	for (int i = 0; i < GetPlayerCount(); i++)
		PlayerActor_unfreeze(GetPtrToPlayerActorByID(i));
}

//============================= World 6: Monty Tank =============================

//On create.
/*
void hook_02134AD0_ov_13()
{

}

//On delete.
void hook_02131074_ov_13()
{
	
}
*/

//============================= World 7: Lakithunder =============================

//Unfreeze both players.
void repl_02131554_ov_11()
{
	for (int i = 0; i < GetPlayerCount(); i++)
		PlayerActor_unfreeze(GetPtrToPlayerActorByID(i));
}

//Force equal zoom
void hook_0212F9B8_ov_11()
{
	int* StageZoomForPlayer = (int*)0x020CADB4;
	StageZoomForPlayer[1] = StageZoomForPlayer[0];
}

void repl_0212FD98_ov_11() { asm("MOVS R0, #0"); }
int repl_02130440_ov_11() { return 0; }
int repl_02130544_ov_11() { return 0; }
int repl_02130560_ov_11() { return 0; }

//Virt51 Closest Player
void repl_02132420_ov_11() {}
void repl_0213242C_ov_11() {}

//Virt50 Closest Player
int repl_021325F0_ov_11(EnemyActor* lakithunder)
{
	asm("MOV R0, R4"); //Place lakithunder ptr in the argument
	return GetPlayerFacingDirection(lakithunder, &lakithunder->actor.position);
}

//Virt49 Closest Player
int repl_021328D0_ov_11(EnemyActor* lakithunder)
{
	asm("MOV R0, R5"); //Place lakithunder ptr in the argument
	return GetPlayerFacingDirection(lakithunder, &lakithunder->actor.position);
}
int repl_021329D8_ov_11(EnemyActor* lakithunder)
{
	asm("PUSH {R1-R12}"); //Save all registers
	asm("MOV R0, R5"); //Place lakithunder ptr in the argument
	int result = GetPlayerFacingDirection(lakithunder, &lakithunder->actor.position);
	asm("POP {R1-R12}"); //Recover all registers
	return result;
}

//============================= World 8: Final Boss Controller =============================

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

				player->actor.position = wall_player->actor.position - (1.5 * 0x10000);
				//player->actor.position = controller->actor.position;
				//player->actor.position.x -= (19 + (i * 3)) * 0x8000;
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

//============================= Main Boss Controller =============================

//Freeze and move player
void repl_021438AC_ov_28(PlayerActor* wall_player)
{
	PlayerActor_freeze(wall_player, 1);

	if (GetPlayerCount() == 2)
	{
		PlayerActor* oppositePlayer = GetPtrToPlayerActorByID(!wall_player->actor.playerNumber);
		PlayerActor_freeze(oppositePlayer, 1);

		oppositePlayer->actor.position.x = wall_player->actor.position.x - (1.5 * 0x10000);
		oppositePlayer->actor.position.y = wall_player->actor.position.y;
	}
}

//============================= Boss Key =============================

//Victory freeze
void repl_0214619C_ov_28()
{
	u8 playerNo = *(u8*)0x020CA298;
	GetPtrToPlayerActorByID(playerNo)->P.physicsStateBitfield |= 0x800000;

	if(GetPlayerCount() == 2)
		GetPtrToPlayerActorByID(!playerNo)->P.jumpBitfield |= 0x1000000;
}