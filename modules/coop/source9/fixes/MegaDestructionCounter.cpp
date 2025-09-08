#include "coop/CoopActor.hpp"
#include "coop/CoopStage.hpp"

extern "C" {
	void CoopSym_MegaDestructionCounter_spawnReward(StageEntity* self, Vec3* position, u32 offset);
}

namespace CoopFixes::MegaDestructionCounter {

bool someoneHasMegaTimer()
{
	for (u32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		if (Game::getMegaTimer(playerID) > 0)
			return true;
	}
	return false;
}

bool someoneHasMegaPowerup()
{
	for (u32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		if (Game::getPlayer(playerID)->currentPowerup == PowerupState::Mega)
			return true;
	}
	return false;
}

bool everyoneMegaDamaged()
{
	for (u32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);
		if (player->currentPowerup != PowerupState::Mega || player->constrainedMovement != PlayerConstrainedMovement::Damaged)
			return false;
	}
	return true;
}

ncp_set_call(0x0215F858, 54, someoneHasMegaTimer)
ncp_set_call(0x0215F98C, 54, someoneHasMegaTimer)

ncp_set_call(0x0215F834, 54, someoneHasMegaPowerup)
ncp_repl(0x0215F838, 54, "NOP; NOP; CMP R0, #1")

ncp_set_call(0x0215F7EC, 54, everyoneMegaDamaged)
ncp_repl(0x0215F7F0, 54, "NOP; CMP R0, #1")

ncp_set_call(0x0215F998, 54, everyoneMegaDamaged)
ncp_repl(0x0215F99C, 54, "NOP; CMP R0, #1")

ncp_call(0x0215F660, 54)
Player* getLevelEndRewardTargetPlayer()
{
	return Game::getPlayer(CoopStage::tempVar);
}

ncp_set_call(0x0215F61C, 54, CoopActor::megaRewardSpawnActor)

/*
extern "C" {
	void CoopSym_MegaDestructionCounter_fun0215FA0C(u32 self, u32);
	extern fx32 CoopSym_Stage_zoom[2];
}

struct Vec3_unsafe
{
	void* vtbl;
	fx32 x;
	fx32 y;
	fx32 z;
};

ncp_jump(0x0215F400, 54)
void spawnRewards(u32 self)
{
	u16& spawnedRewardBits = *rcast<u16*>(self + 126);
	u8& spawnedRewardCount = *rcast<u8*>(self + 136);
	auto& spawnReward = *rcast<void(**)(u32 self, Vec3* position, u32 offset)>(self + 156);

	if (spawnedRewardCount < 5)
		spawnedRewardBits &= ~(1 << spawnedRewardCount);
	spawnedRewardCount = -1;

	if (!spawnedRewardBits)
	{
		CoopSym_MegaDestructionCounter_fun0215FA0C(self, 2);
		return;
	}

	u32 offset = 0;
	while ((spawnedRewardBits & (1 << offset)) == 0)
	{
		if (++offset >= 5)
			return;
	}

	for (u32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		if (Game::getPlayerDead(playerID) || CoopStage::playerInFlagpoleSentFlying[playerID])
			continue;

		fx32 cameraY = Stage::cameraY[playerID];
		fx32 cameraX = Stage::cameraX[playerID];
		fx32 zoom = CoopSym_Stage_zoom[playerID];

		fx32 zoomScaleY = FX_Div(FX32_ONE, zoom);

		Vec3_unsafe position;
		position.x = cameraX;
		position.y = 0x8000 - FX_Mul(zoomScaleY, 0x8000) - cameraY;
		position.z = 0x100000;

		s16* rewardSpawnOffsets = rcast<s16*>(self + 100 + 4 * offset);

		fx32 rewardSpawnOffsetX = rewardSpawnOffsets[0] << FX32_SHIFT;
		fx32 rewardSpawnOffsetY = (((*rcast<s16*>(self + 122) + rewardSpawnOffsets[1]) << FX32_SHIFT) + 0x8000);

		position.x = cameraX + FX_Mul(rewardSpawnOffsetX, zoom);
		position.y -= FX_Mul(rewardSpawnOffsetY, zoom);

		CoopStage::tempVar = playerID; // Spawned reward must track the correct player
		spawnReward(self, rcast<Vec3*>(&position), offset);
	}

	spawnedRewardCount = offset;
}
*/

ncp_over(0x0215F400, 54) /* max over: 0x1A0 bytes, current: 0x17C bytes */
ncp_asmfunc void spawnRewards_ASM()
{asm(R"(
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	ldrb	r2, [r0, #136]
	sub	sp, sp, #20
	cmp	r2, #4
	movls	r1, #1
	ldrhls	r3, [r0, #126]
	mov	r4, r0
	bicls	r3, r3, r1, lsl r2
	strhls	r3, [r0, #126]
	mvn	r3, #0
	strb	r3, [r0, #136]
	ldrh	r3, [r0, #126]
	cmp	r3, #0
	movne	r6, #0
	bne	.mdcsi_L28
	mov	r1, #2
	add	sp, sp, #20
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	b	CoopSym_MegaDestructionCounter_fun0215FA0C
.mdcsi_L30:
	add	r6, r6, #1
	cmp	r6, #5
	bne	.mdcsi_L28
.mdcsi_L26:
	add	sp, sp, #20
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.mdcsi_L28:
	asr	r2, r3, r6
	tst	r2, #1
	beq	.mdcsi_L30
	mov	r5, #0
	add	r8, r4, #156
.mdcsi_L31:
	bl	_ZN4Game14getPlayerCountEv
	cmp	r0, r5
	strbls	r6, [r4, #136]
	bls	.mdcsi_L26
.mdcsi_L33:
	mov	r0, r5
	bl	_ZN4Game13getPlayerDeadEl
	cmp	r0, #0
	bne	.mdcsi_L32
	ldr	r3, .mdcsi_L40
	ldrb	r7, [r3, r5]
	cmp	r7, #0
	bne	.mdcsi_L32
	ldr	r3, .mdcsi_L40+4
	mov	r0, #4096
	ldr	r10, [r3, r5, lsl #2]
	ldr	r3, .mdcsi_L40+8
	ldr	r9, [r3, r5, lsl #2]
	ldr	r3, .mdcsi_L40+12
	ldr	fp, [r3, r5, lsl #2]
	mov	r1, fp
	bl	FX_Div
	mov	r3, #1048576
	add	r1, r4, #100
	add	ip, r1, r6, lsl #2
	ldrsh	ip, [ip, #2]
	ldrsh	r2, [r4, #122]
	str	r3, [sp, #12]
	lsl	r3, r6, #2
	add	r2, r2, ip
	ldrsh	ip, [r3, r1]
	mov	r3, #2048
	mov	lr, r7
	mov	r1, r3
	lsl	ip, ip, #12
	smlal	r1, lr, ip, fp
	lsl	r2, r2, #12
	lsr	r1, r1, #12
	add	r2, r2, #32768
	orr	r1, r1, lr, lsl #20
	add	r9, r9, r1
	add	r1, r7, r0, asr #17
	add	r0, r3, r0, lsl #15
	smlal	r3, r7, r2, fp
	lsr	r0, r0, #12
	orr	r0, r0, r1, lsl #20
	rsb	r0, r0, #32768
	lsr	r3, r3, #12
	orr	r3, r3, r7, lsl #20
	sub	r0, r0, r10
	sub	r0, r0, r3
	ldr	r3, .mdcsi_L40+16
	str	r0, [sp, #8]
	str	r5, [r3]
	mov	r2, r6
	mov	r1, sp
	mov	r0, r4
	ldr	r3, [r8]
	str	r9, [sp, #4]
	blx	r3
.mdcsi_L32:
	add	r5, r5, #1
	b	.mdcsi_L31
.mdcsi_L40:
	.word	_ZN9CoopStage26playerInFlagpoleSentFlyingE
	.word	_ZN5Stage7cameraYE
	.word	_ZN5Stage7cameraXE
	.word	CoopSym_Stage_zoom
	.word	_ZN9CoopStage7tempVarE
)");}

// Shared mega destruction score

ncp_repl(0x020200E0, "MOV R3, #0")
ncp_repl(0x020200E8, "MOV LR, R12")
ncp_repl(0x02020118, "MOV R0, #0")
ncp_repl(0x0202012C, "MOV R0, #0")

} // namespace CoopFixes::MegaDestructionCounter
