#include "nsmb.h"

//Add model 3d to Vs Battle Star struct
struct VsBattleStar_3D {
	EnemyActor enemy;
	u8 unk[0xFC];
	Model3d model;
};

//Expand allocation size of the Vs Battle Star
int repl_0213271C_ov_14() { return sizeof(VsBattleStar_3D); }
int repl_021326F0_ov_14() { return sizeof(VsBattleStar_3D); }

void hook_0213190C_ov_14(VsBattleStar_3D* battleStar)
{
	nFS_LoadFileByIDToCache(2089 - 131, false);
	model3d_ctor(&battleStar->model);
	model3d_setup(&battleStar->model, nFS_GetPtrToCachedFile(2089 - 131), 0);
	model3d_init(&battleStar->model);
}

int nsub_021316A8_ov_14(VsBattleStar_3D* battleStar)
{
	if (((u16*)battleStar)[625])
		return 1;

	Vec3 drawPos = battleStar->enemy.actor.position;
	drawPos.y += 0x10000;

	S16Vec3 drawRot = S16Vec3(0, 0, -((u16*)battleStar)[80]);

	model3d_draw(&battleStar->model, &drawPos, &drawRot, &battleStar->enemy.actor.scale);

	return 1;
}

int repl_0209C4A0_ov_00() { return 1; }
void repl_021316C8_ov_14() { asm("B 0x021316E4"); }
int repl_0209E14C_ov_00() { return 1; } //Allow player to spawn battle stars on damage
int repl_02132584_ov_14() { return 0; } //Needs a look into