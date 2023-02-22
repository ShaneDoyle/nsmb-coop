#include "nsmb/game.h"
#include "nsmb/sound.h"
#include "nsmb/stage/entity3danm.h"

// Replacement for StageEntity::skipRender that updates the model but doesn't draw it
static bool ActorFixes_safeSkipRender(StageEntity3DAnm* self)
{
	self->StageEntity::skipRender() ?
		self->model.disableRendering() :
		self->model.enableRendering();
	return false;
}

// Hammer/Fire/Boomerang Bros ---

ncp_over(0x021754F8, 56) const auto HammerBro_skipRender = ActorFixes_safeSkipRender;
ncp_over(0x02175730, 56) const auto FireBro_skipRender = ActorFixes_safeSkipRender;
ncp_over(0x02175614, 56) const auto BoomerangBro_skipRender = ActorFixes_safeSkipRender;

// Red Ring ----------------

/*
struct RedRingVec3_unsafe
{
void* vtbl;
fx32 x;
fx32 y;
fx32 z;
};

ncp_call(0x02153F30, 54)
void RedRing_spawnItem(StageEntity* self)
{
	Player *player;
	int old_powerup;
	int new_powerup;
	RedRingVec3_unsafe item_pos;

	item_pos.z = 0;

	for (int i = 0; i < Game::getPlayerCount(); i++)
	{
		if (Game::getPlayerDead(i))
			continue;

		player = Game::getPlayer(i);

		item_pos.x = player->position.x & Game::wrapX;
		item_pos.y = -0x2000u - Stage::cameraY[i];

		old_powerup = (int)player->currentPowerup;

		if (old_powerup && old_powerup != 4)
		{
			if (old_powerup == 1)
				new_powerup = 1;
			else
				new_powerup = 2;
		}
		else
		{
			new_powerup = 0;
		}

		if (new_powerup == 2 && i == Game::localPlayerID)
			SND::playSFX(0x17E, &self->position);

		Actor::spawnActor(0x1Fu, ((u32*)0x216BFB8)[new_powerup] | (i << 16), (Vec3*)&item_pos, 0, 0, 0);
	}
}
*/

asm(R"(
ncp_over(0x0215410C, 54) /* max over: 0xFC bytes, current: 0xF0 bytes */
	push	{r4, r5, r6, r7, r8, lr}
	mov	r4, #0
	mov	r5, r0
	sub	sp, sp, #24
	ldr	r6, .L10
	ldr	r7, .L10+4
	ldr	r8, .L10+8
	str	r4, [sp, #20]
.L2:
	bl	_ZN4Game14getPlayerCountEv
	cmp	r0, r4
	bgt	.L6
	add	sp, sp, #24
	pop	{r4, r5, r6, r7, r8, pc}
.L6:
	mov	r0, r4
	bl	_ZN4Game13getPlayerDeadEl
	cmp	r0, #0
	bne	.L3
	mov	r0, r4
	bl	_ZN4Game9getPlayerEl
	ldr	r2, [r6]
	ldr	r3, [r0, #96]
	add	r0, r0, #1952
	and	r3, r3, r2
	str	r3, [sp, #12]
	ldr	r2, [r7, r4, lsl #2]
	ldr	r3, .L10+12
	sub	r3, r3, r2
	str	r3, [sp, #16]
	ldrsb	r3, [r0, #12]
	bics	r2, r3, #4
	moveq	r3, r2
	beq	.L4
	cmp	r3, #1
	beq	.L4
	ldr	r3, .L10+16
	ldr	r3, [r3]
	cmp	r4, r3
	bne	.L5
	ldr	r0, .L10+20
	add	r1, r5, #92
	bl	_ZN3SND7playSFXElPK4Vec3
.L5:
	mov	r3, #2
.L4:
	add	r3, r3, r8
	lsl	r3, r3, #2
	ldr	r1, [r3]
	mov	r3, #0
	mov	r0, #31
	str	r3, [sp, #4]
	str	r3, [sp]
	add	r2, sp, #8
	orr	r1, r1, r4, lsl #16
	bl	_ZN5Actor10spawnActorEtmPK4Vec3PK5Vec3sPKlPKa
.L3:
	add	r4, r4, #1
	b	.L2
.L10:
	.word	_ZN4Game5wrapXE
	.word	_ZN5Stage7cameraYE
	.word	8761326
	.word	-8192
	.word	_ZN4Game13localPlayerIDE
	.word	382
ncp_endover()
)");
