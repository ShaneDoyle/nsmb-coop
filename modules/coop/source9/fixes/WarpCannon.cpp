#include <nsmb/game/game.hpp>
#include <nsmb/game/sound.hpp>
#include <nsmb/game/stage.hpp>
#include <nsmb/core/system/function.hpp>

#include "coop/CoopActor.hpp"
#include "coop/CoopCamera.hpp"

struct WarpCannon_PTMF
{
	bool (*func)(StageEntity*);
	u32 adj;
};

extern "C"
{
	bool CoopSym_WarpCannon_switchState(StageEntity* self, WarpCannon_PTMF* ptmf);
	extern WarpCannon_PTMF CoopSym_WarpCannon_sAfterShoot;
}

namespace CoopFixes::WarpCannon {

ncp_over(0x0217FD74, 89) const auto vtbl_skipRender = CoopActor::safeSkipRender;

ncp_repl(0x0217F6C8, 89, "MOV R2, #0") // Force player 0 to be shot

ncp_thumb bool shootOtherPlayersState(StageEntity* self)
{
	const u32 ShootInterval = 10;

	ModelAnm& modelAnm = *rcast<ModelAnm*>(rcast<s8*>(self) + 0x48C);
	u32& waitingInCannon = rcast<u32*>(self)[0x6D8 / 4];
	u16& playersShot = rcast<u16*>(self)[0x6E8 / 2];
	s8& step = rcast<s8*>(self)[0x6F5];

	if (step == Func::Init)
	{
		step++;
		waitingInCannon = 0; // Stop updating player 0
		playersShot = 1; // Player 0 already shot by the time it reaches here

		return true;
	}

	if (step == Func::Exit)
	{
		return true;
	}

	modelAnm.frameController.update();

	if (step < ShootInterval + 1)
	{
		step++;
		return true;
	}

	if (playersShot == Game::getPlayerCount())
	{
		CoopSym_WarpCannon_switchState(self, &CoopSym_WarpCannon_sAfterShoot);
		return true;
	}

	step = 1;

	Player* player = Game::getPlayer(playersShot);
	player->visible = true;

	s16 angleX = -rcast<s16*>(self)[0x6E4 / 2] - (0x200 * playersShot);
	s16 angleY = rcast<s16*>(self)[0x6E0 / 2] + 0x4400;
	Vec3& position = *rcast<Vec3*>(rcast<u8*>(self) + 0x534);

	player->waitInCannon(*self, position, angleX, angleY);
	player->shootFromCannon(*self, 4fx, angleX, 0x6000, 1);

	if (Game::getPlayerCharacter(playersShot) == 0)
		SND::playSFX(68, &self->position);
	else
		SND::playSFX(320, &self->position);

	playersShot++;

	return true;
}

static WarpCannon_PTMF WarpCannon_sShootOtherPlayers = { shootOtherPlayersState, 0 };

ncp_call(0x0217F218, 89)
ncp_thumb void customSwitchStateAfterShoot(StageEntity* self, WarpCannon_PTMF* ptmf)
{
	if (Game::getPlayerCount() == 1)
	{
		CoopSym_WarpCannon_switchState(self, ptmf); // ptmf == WarpCannon_sAfterShoot
		return;
	}

	Game::getPlayer(0)->visible = true;

	// Match player 0's camera
	for (s32 playerID = 1; playerID < Game::getPlayerCount(); playerID++)
		CoopCamera::setFollowTarget(playerID, 0);

	CoopSym_WarpCannon_switchState(self, &WarpCannon_sShootOtherPlayers);
}

ncp_call(0x0217F70C, 89)
ncp_thumb void customSwitchToRotatingState(StageEntity* self, WarpCannon_PTMF* ptmf)
{
	CoopSym_WarpCannon_switchState(self, ptmf);

	if (Game::getPlayerCount() == 1)
		return;

	// Hide the players because they stack up for whatever reason
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);
		player->visible = false;
	}
}

ncp_asmfunc void playerEntryFix_ASM()
{asm(R"(
// Allow more than 1 player to enter the cannon
ncp_jump(0x0217F618, 89)
	LDRH    R1, [R0,#0xE8]
	ADD     R1, R1, #1
	B       0x0217F61C

ncp_jump(0x0217F5E0, 89)
	LDR     R1, =_ZN4Game11playerCountE
	LDR     R1, [R1]
	CMP     R0, R1
	BEQ     0x0217F6B4
	B       0x0217F5E8
)");};

} // namespace CoopFixes::WarpCannon
