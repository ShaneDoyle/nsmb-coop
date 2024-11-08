#include "nsmb/game.hpp"
#include "nsmb/sound.hpp"
#include "nsmb/stage/entity3danm.hpp"
#include "nsmb/stage/viewshaker.hpp"
#include "nsmb/system/function.hpp"

#include "ActorFixes.hpp"
//#include "PlayerSpectate.hpp"

asm(R"(
	SledgeBro_tryShakePlayer = 0x02174DE4
)");
extern "C" {
	void SledgeBro_tryShakePlayer(StageEntity* self, s32 playerID);
}

// Replacement for StageEntity::skipRender that updates the model but doesn't draw it
bool ActorFixes_safeSkipRender(StageEntity3DAnm* self)
{
	self->StageEntity::skipRender() ?
		self->model.disableRendering() :
		self->model.enableRendering();
	return false;
}

// Replacement for Game::getLocalPlayer in some cases
Player* ActorFixes_getClosestPlayer(StageEntity* self)
{
	return self->getClosestPlayer(nullptr, nullptr);
}

// Replacement for Game::isOutsideCamera(..., Game::localPlayerID)
bool ActorFixes_isOutsideCamera(StageEntity* self, const FxRect& boundingBox, u8 playerID)
{
	Player* player = ActorFixes_getClosestPlayer(self);
	return Stage::isOutsideCamera(self->position, boundingBox, player->linkedPlayerID);
}

static u32 ActorFixes_matchZoneID = -1;

NTR_USED static Player* ActorFixes_getClosestPlayerFilter(s32 playerID)
{
	Player* player = Game::getPlayer(playerID);
	if (player == nullptr || (ActorFixes_matchZoneID != -1 && !ActorFixes_isPlayerInZone(player, ActorFixes_matchZoneID)))
		return nullptr;
	return player;
}

ncp_set_call(0x020A0544, 0, ActorFixes_getClosestPlayerFilter)
ncp_set_call(0x020A0628, 0, ActorFixes_getClosestPlayerFilter)

Player* ActorFixes_getClosestPlayerInZone(StageEntity* self, u32 zoneID)
{
	ActorFixes_matchZoneID = zoneID;
	Player* player = ActorFixes_getClosestPlayer(self);
	ActorFixes_matchZoneID = -1;
	return player;
}

// Hammer/Fire/Boomerang Bros -----------------------------------------------------------

ncp_over(0x021754F8, 56) const auto HammerBro_skipRender = ActorFixes_safeSkipRender;
ncp_over(0x02175730, 56) const auto FireBro_skipRender = ActorFixes_safeSkipRender;
ncp_over(0x02175614, 56) const auto BoomerangBro_skipRender = ActorFixes_safeSkipRender;

// Sledge Bro -----------------------------------------------------------

ncp_over(0x02175880, 56) const auto SledgeBro_skipRender = ActorFixes_safeSkipRender;

static bool SledgeBro_canTryShakePlayer(StageEntity* self, Player* player)
{
	const fx32 range = 0x100000; // 16 tiles

	bool inRange = Math::abs(self->position.x - player->position.x) < range &&
	               Math::abs(self->position.y - player->position.y) < range;

	return inRange && !Game::getPlayerDead(player->linkedPlayerID);
}

NTR_USED static void SledgeBro_fixShakePlayer(StageEntity* self)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);

		if (SledgeBro_canTryShakePlayer(self, player))
		{
			ViewShaker::start(3, self->viewID, playerID, false);
			if (playerID == Game::localPlayerID)
				SND::playSFX(138, &self->position);
			SledgeBro_tryShakePlayer(self, playerID);
		}
	}
}

ncp_repl(0x02174614, 56, R"(
	MOV     R0, R4
	BL      _ZL24SledgeBro_fixShakePlayerP11StageEntity
	B       0x02174658
)");

// Dorrie -------------------------------------------------------------------------------

ncp_over(0x02148348, 47) const auto Dorrie_skipRender = ActorFixes_safeSkipRender;

// Red Ring -----------------------------------------------------------------------------

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
	ldr	r6, .ring_L10
	ldr	r7, .ring_L10+4
	ldr	r8, .ring_L10+8
	str	r4, [sp, #20]
.ring_L2:
	bl	_ZN4Game14getPlayerCountEv
	cmp	r0, r4
	bgt	.ring_L6
	add	sp, sp, #24
	pop	{r4, r5, r6, r7, r8, pc}
.ring_L6:
	mov	r0, r4
	bl	_ZN4Game13getPlayerDeadEl
	cmp	r0, #0
	bne	.ring_L3
	mov	r0, r4
	bl	_ZN4Game9getPlayerEl
	ldr	r2, [r6]
	ldr	r3, [r0, #96]
	add	r0, r0, #1952
	and	r3, r3, r2
	str	r3, [sp, #12]
	ldr	r2, [r7, r4, lsl #2]
	ldr	r3, .ring_L10+12
	sub	r3, r3, r2
	str	r3, [sp, #16]
	ldrsb	r3, [r0, #12]
	bics	r2, r3, #4
	moveq	r3, r2
	beq	.ring_L4
	cmp	r3, #1
	beq	.ring_L4
	ldr	r3, .ring_L10+16
	ldr	r3, [r3]
	cmp	r4, r3
	bne	.ring_L5
	ldr	r0, .ring_L10+20
	add	r1, r5, #92
	bl	_ZN3SND7playSFXElPK4Vec3
.ring_L5:
	mov	r3, #2
.ring_L4:
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
.ring_L3:
	add	r4, r4, #1
	b	.ring_L2
.ring_L10:
	.word	_ZN4Game5wrapXE
	.word	_ZN5Stage7cameraYE
	.word	8761326
	.word	-8192
	.word	_ZN4Game13localPlayerIDE
	.word	382
ncp_endover()
)");

// Lava (234) & Poisoned Water (259) ----------------------------------------------------

ncp_repl(0x020BBE88, 0, "NOP") // Prevent liquid type change on respawn

asm(R"(
ncp_call(0x020FE2D8, 10)
ncp_call(0x020FEB68, 10)
ncp_call(0x021033EC, 10)
ncp_call(0x0210E6AC, 10)
	LDR     R1, =_ZN4Game13localPlayerIDE
	LDR     R1, [R1]
	LDRB    R0, [R0,R1]
	BX      LR

// There is a non-local player access at 0x020A6EA4, but the code never reaches it
// Just in case, make it return -1 even if a playerID is specified
ncp_over(0x020A6E9C, 0)
	MOV     R0, #1
	NOP
	NOP
ncp_endover()
)");

// TODO: LIQUID POSITION AND LAST LIQUID POSITION

// Balloon Boo --------------------------------------------------------------------------

ncp_repl(0x0217716C, 71, "NOP") // Pass Boo* instead of &Boo*->position
ncp_set_call(0x02177178, 71, ActorFixes_isOutsideCamera)

// Rotating Carry Through Wall Platform -------------------------------------------------

ncp_over(0x0218FF54, 118) const auto RotatingCarryThroughWallPlatform_skipRender = ActorFixes_safeSkipRender;

// Blockhopper --------------------------------------------------------------------------

ncp_over(0x021784B8, 69) const auto Blockhopper_skipRender = ActorFixes_safeSkipRender;

asm(R"(
ncp_jump(0x02177260, 69)
	MOV     R0, R4
	BL      _Z27ActorFixes_getClosestPlayerP11StageEntity
	B       0x02177264

ncp_over(0x021778A4, 69)
	NOP
	MOV     R0, R4
	BL      _Z27ActorFixes_getClosestPlayerP11StageEntity
ncp_endover()
)");

ncp_set_call(0x02177450, 69, ActorFixes_getClosestPlayer)

// Flying Red Block ---------------------------------------------------------------------

asm(R"(
ncp_over(0x0215BBE4, 54)
	MOV     R4, R0
	BL      _Z27ActorFixes_getClosestPlayerP11StageEntity
	NOP
	NOP
ncp_endover()

ncp_over(0x0215BC48, 54)
	MOV     R4, R0
	BL      _Z27ActorFixes_getClosestPlayerP11StageEntity
	NOP
	NOP
ncp_endover()

ncp_over(0x0215BCCC, 54)
	MOV     R0, R4
	BL      _Z27ActorFixes_getClosestPlayerP11StageEntity
	NOP
ncp_endover()
)");

// Warp Cannon ---------------------------------------------------------------------

struct WarpCannon_PTMF
{
	bool (*func)(StageEntity*);
	u32 adj;
};

asm(R"(
	WarpCannon_sAfterShoot = 0x0217FE60
	WarpCannon_switchState = 0x0217F7D4
)");

extern "C"
{
	WarpCannon_PTMF WarpCannon_sAfterShoot;
	bool WarpCannon_switchState(StageEntity* self, WarpCannon_PTMF* ptmf);
}

ncp_repl(0x0217F6C8, 89, "MOV R2, #0") // Force player 0 to be shot

bool WarpCannon_shootOtherPlayersState(StageEntity* self)
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

		//for (s32 playerID = 1; playerID < Game::getPlayerCount(); playerID++)
		//	PlayerSpectate::setTarget(playerID, 0);

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
		WarpCannon_switchState(self, &WarpCannon_sAfterShoot);
		return true;
	}

	step = 1;

	Player* player = Game::getPlayer(playersShot);

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

static WarpCannon_PTMF WarpCannon_sShootOtherPlayers = { WarpCannon_shootOtherPlayersState, 0 };

ncp_call(0x0217F218, 89)
void WarpCannon_customSwitchStateAfterShoot(StageEntity* self, WarpCannon_PTMF* ptmf)
{
	if (Game::getPlayerCount() == 1)
	{
		WarpCannon_switchState(self, ptmf); // ptmf == WarpCannon_sAfterShoot
		return;
	}

	WarpCannon_switchState(self, &WarpCannon_sShootOtherPlayers);
}

asm(R"(
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
)");

// Misc ---------------------------------------------------------------------------------

//Fix Volcano BG                                    MUST BE CHECKED LATER
// void repl_020B6B6C() { asm("MOV R1, #2"); }

//Remove delete range for UnusedSpikeBass (256).    PATCH ADDRESS IS INVALID
//void repl_021728EC_ov_3A() {}

// Fix Rotating Barrel (246) Desync.
ncp_repl(0x02186F58, 96, "MOV LR, #0")

// Fix horizontal movement mushroom. (DeleteIfOutOfRange) (PROBABLY DOESN'T WORK)
//void repl_0217FDDC_ov_5A() {}

// Fix pipe cannon desync.
ncp_repl(0x020F8230, 10, "B 0x020F823C")







// TODO: CHECK WHAT 020D98DC IS
