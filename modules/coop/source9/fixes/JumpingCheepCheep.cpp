#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/player/player.hpp>

#include "coop/CoopActor.hpp"

namespace CoopFixes::JumpingCheepCheep {

u32 cameraX;
u32 cameraWidth;
u32 cameraVelX;

void updateVars(StageEntity* self)
{
	s32 playerID = CoopActor::getClosestPlayer(self)->linkedPlayerID;
	cameraX = Stage::cameraX[playerID];
	cameraWidth = Stage::cameraWidth[playerID];
	cameraVelX = rcast<Vec3*>(0x020CAED8)[playerID].x;
}

ncp_set_hook(0x02147868, 50, updateVars)
ncp_repl(0x02147D10, 50, ".int _ZN9CoopFixes17JumpingCheepCheep7cameraXE")
ncp_repl(0x02147D14, 50, ".int _ZN9CoopFixes17JumpingCheepCheep11cameraWidthE")
ncp_repl(0x02147D30, 50, ".int _ZN9CoopFixes17JumpingCheepCheep10cameraVelXE - 4")

ncp_set_call(0x0214762C, 50, updateVars)
ncp_repl(0x02147840, 50, ".int _ZN9CoopFixes17JumpingCheepCheep10cameraVelXE - 4")

} // namespace CoopFixes::JumpingCheepCheep
