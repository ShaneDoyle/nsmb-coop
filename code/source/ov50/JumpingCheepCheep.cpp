#include <nsmb/nm/stage/entity.hpp>
#include <nsmb/nm/stage/player/player.hpp>

#include "ActorFixes.hpp"

// Jumping Cheep Cheep ------------------------------------------------------------------

NTR_USED static u32 JumpingCheepCheep_cameraX;
NTR_USED static u32 JumpingCheepCheep_cameraWidth;
NTR_USED static u32 JumpingCheepCheep_cameraUnk;

NTR_USED static void JumpingCheepCheep_updateVars(StageEntity* self)
{
	s32 playerID = ActorFixes_getClosestPlayer(self)->linkedPlayerID;
	JumpingCheepCheep_cameraX = Stage::cameraX[playerID];
	JumpingCheepCheep_cameraWidth = Stage::cameraWidth[playerID];
	JumpingCheepCheep_cameraUnk = rcast<Vec3*>(0x020CAED8)[playerID].x;
}

ncp_set_hook(0x02147868, 50, JumpingCheepCheep_updateVars)
ncp_repl(0x02147D10, 50, ".int _ZL25JumpingCheepCheep_cameraX")
ncp_repl(0x02147D14, 50, ".int _ZL29JumpingCheepCheep_cameraWidth")
ncp_repl(0x02147D30, 50, ".int _ZL27JumpingCheepCheep_cameraUnk - 4")

ncp_set_call(0x0214762C, 50, JumpingCheepCheep_updateVars)
ncp_repl(0x02147840, 50, ".int _ZL27JumpingCheepCheep_cameraUnk - 4")
