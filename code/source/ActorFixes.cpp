#include "nsmb/game.h"
#include "nsmb/sound.h"
#include "nsmb/stage/entity3danm.h"
#include "nsmb/stage/viewshaker.h"

asm(R"(
	SledgeBro_tryShakePlayer = 0x02174DE4
)");
extern "C" {
	void SledgeBro_tryShakePlayer(StageEntity* _this, s32 playerID);
}

// Replacement for StageEntity::skipRender that updates the model but doesn't draw it
static bool ActorFixes_safeSkipRender(StageEntity3DAnm* self)
{
	self->StageEntity::skipRender() ?
		self->model.disableRendering() :
		self->model.enableRendering();
	return false;
}

// Replacement for Game::getLocalPlayer in some cases
NTR_USED static Player* ActorFixes_getClosestPlayer(StageEntity* self)
{
	return self->getClosestPlayer(nullptr, nullptr);
}

// Replacement for Game::isOutsideCamera(..., Game::localPlayerID)
NTR_USED static bool ActorFixes_isOutsideCamera(StageEntity* self, const FxRect& boundingBox, u8 playerID)
{
	Player* player = ActorFixes_getClosestPlayer(self);
	return Stage::isOutsideCamera(self->position, boundingBox, player->linkedPlayerID);
}

static u32 ActorFixes_matchZoneID = -1;

NTR_USED static Player* ActorFixes_getClosestPlayerFilter(s32 playerID)
{
	Player* player = Game::getPlayer(playerID);
	if (player == nullptr || (ActorFixes_matchZoneID != -1 && !(scast<StageEntity*>(nullptr))->isPlayerInZone(*player, ActorFixes_matchZoneID)))
		return nullptr;
	return player;
}

ncp_set_call(0x020A0544, 0, ActorFixes_getClosestPlayerFilter)
ncp_set_call(0x020A0628, 0, ActorFixes_getClosestPlayerFilter)

NTR_USED static Player* ActorFixes_getClosestPlayerInZone(StageEntity* self, u32 zoneID)
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
	BL      _ZL27ActorFixes_getClosestPlayerP11StageEntity
	B       0x02177264

ncp_over(0x021778A4, 69)
	NOP
	MOV     R0, R4
	BL      _ZL27ActorFixes_getClosestPlayerP11StageEntity
ncp_endover()
)");

ncp_set_call(0x02177450, 69, ActorFixes_getClosestPlayer)

// Flying Red Block ---------------------------------------------------------------------

asm(R"(
ncp_over(0x0215BBE4, 54)
	MOV     R4, R0
	BL      _ZL27ActorFixes_getClosestPlayerP11StageEntity
	NOP
	NOP
ncp_endover()

ncp_over(0x0215BC48, 54)
	MOV     R4, R0
	BL      _ZL27ActorFixes_getClosestPlayerP11StageEntity
	NOP
	NOP
ncp_endover()

ncp_over(0x0215BCCC, 54)
	MOV     R0, R4
	BL      _ZL27ActorFixes_getClosestPlayerP11StageEntity
	NOP
ncp_endover()
)");

// Unagi Eel ----------------------------------------------------------------------------

ncp_repl(0x02179A20, 79, "MOV R0, R4") // Pass Unagi* instead of &Unagi*->position
ncp_set_call(0x02179A44, 79, ActorFixes_safeSkipRender)
ncp_set_call(0x02179A28, 79, ActorFixes_isOutsideCamera)

asm(R"(
ncp_call(0x0217ABA4, 79)
ncp_call(0x0217AEC4, 79)
	MOV     R0, R4
	B       _ZL27ActorFixes_getClosestPlayerP11StageEntity

ncp_jump(0x0217B8AC, 79) // Mega bump fix
	PUSH    {R4,LR}
	MOV     R4, R0
	B       0x0217B8B0
)");

ncp_repl(0x0217B900, 79, "POP {R4,PC}") // Mega bump fix

ncp_repl(0x0217B8C8, 79, "ADD R0, R4, #0x100") // Mega bump fix
ncp_repl(0x0217B8D0, 79, "LDRSB R0, [R0,#0x1E]") // Mega bump fix

// Spike Bass ---------------------------------------------------------------------------

ncp_repl(0x021731B4, 58, ".int 0x4BC") // Add a zoneID field

NTR_USED static Player* SpikeBass_fixGetClosestPlayer(StageEntity* self)
{
	u32 zoneID = rcast<u32*>(self)[0x4B8 / 4];
	Player* player = ActorFixes_getClosestPlayerInZone(self, zoneID);
	return player ? player : ActorFixes_getClosestPlayer(self);
}

ncp_set_call(0x02172CB0, 58, SpikeBass_fixGetClosestPlayer)
ncp_set_call(0x02172E4C, 58, SpikeBass_fixGetClosestPlayer)

NTR_USED static Player* SpikeBassSpawner_fixCheckPlayerInView(StageEntity* self)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);
		if (player->viewID == self->viewID)
		{
			return player;
		}
	}
	return nullptr;
}

asm(R"(
ncp_call(0x02173318, 58)
	MOV     R0, R4
	B       _ZL37SpikeBassSpawner_fixCheckPlayerInViewP11StageEntity

ncp_jump(0x021734C8, 58)
	STR     R2, [R0,#0x4AC] // Keep replaced instruction
	LDRB    R2, [R4,#0x402] // R2 = SpikeBassSpawner*->zoneID
	STR     R2, [R0,#0x4B8] // SpikeBass*->zoneID = R2
	B       0x021734CC      // Return to code
)");

ncp_call(0x02173370, 58)
bool SpikeBassSpawner_fixCheckPlayerInZone(StageEntity* self, Player* player, u32 zoneID)
{
	return ActorFixes_getClosestPlayerInZone(self, zoneID) != nullptr;
}

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
