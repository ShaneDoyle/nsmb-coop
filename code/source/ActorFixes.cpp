#include <nsmb/game/game.hpp>
#include <nsmb/game/sound.hpp>
#include <nsmb/game/stage/player/player.hpp>
#include <nsmb/game/stage/entity3danm.hpp>
#include <nsmb/game/stage/viewshaker.hpp>
#include <nsmb/game/stage/effect.hpp>
#include <nsmb/game/stage/actors/ov54/warpentrance.hpp>
#include <nsmb/game/stage/actors/ov54/volcanoeruption.hpp>
#include <nsmb/game/stage/actors/ov66/lakituspawner.hpp>
#include <nsmb/core/system/function.hpp>
#include <nsmb/core/math/math.hpp>

#include "ActorFixes.hpp"
#include "Stage.hpp"
//#include "PlayerSpectate.hpp"

// Notes:
//  - Any actor using `Model::getNodePosition` (0x020196DC) or `Model::getNodeMatrix` (0x0201972C) that does
//    not override `skipRender` to always render but become invisible must use `ActorFixes_safeSkipRender`.
//  - Any actor using `Game::isOutsideCamera(..., Game::localPlayerID)` (0x0200AE9C)
//    to make decisions beyond just rendering must use `ActorFixes_isOutsideCamera`.

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
Player* ActorFixes_getClosestPlayer(StageActor* self)
{
	return self->getClosestPlayer(nullptr, nullptr);
}

// Replacement for Game::isOutsideCamera(..., Game::localPlayerID)
bool ActorFixes_isOutsideCamera(StageActor* self, const FxRect& boundingBox/*, u8 playerID*/)
{
	Player* player = ActorFixes_getClosestPlayer(self);
	return Stage::isOutsideCamera(self->position, boundingBox, player->linkedPlayerID);
}

static u32 ActorFixes_matchZoneID = -1;

NTR_USED static Player* ActorFixes_getClosestPlayerFilter(s32 playerID)
{
	if (Game::getPlayerDead(playerID))
		return nullptr;
	Player* player = Game::getPlayer(playerID);
	if (player == nullptr || (ActorFixes_matchZoneID != -1 && !ActorFixes_isPlayerInZone(player, ActorFixes_matchZoneID)))
		return nullptr;
	return player;
}

ncp_set_call(0x020A0544, 0, ActorFixes_getClosestPlayerFilter)
ncp_set_call(0x020A0628, 0, ActorFixes_getClosestPlayerFilter)

Player* ActorFixes_getClosestPlayerInZone(StageActor* self, u32 zoneID)
{
	ActorFixes_matchZoneID = zoneID;
	Player* player = ActorFixes_getClosestPlayer(self);
	ActorFixes_matchZoneID = -1;
	return player;
}

asm(R"(
.type StageActor_getClosestPlayer_SUPER, %function
StageActor_getClosestPlayer_SUPER:
	PUSH    {R4,LR}
	B       0x020A06A0
)");
extern "C" {
	Player* StageActor_getClosestPlayer_SUPER(StageActor* self, s32* distanceX, s32* distanceY);
}

ncp_jump(0x020A069C, 0)
Player* StageActor_getClosestPlayer_OVERRIDE(StageActor* self, s32* distanceX, s32* distanceY)
{
	Player* player = StageActor_getClosestPlayer_SUPER(self, distanceX, distanceY);
	if (player == nullptr && ActorFixes_matchZoneID == -1)
		return Game::getPlayer(0);
	return player;
}

bool ActorFixes_isPlayerInShakeRange(StageActor* self, Player* player)
{
	const fx32 range = 0x100000; // 16 tiles

	bool inRange = Math::abs(self->position.x - player->position.x) < range &&
	               Math::abs(self->position.y - player->position.y) < range;

	return inRange;
}

// Replacement for StageEntity::skipRender when update logic depends on skipRender,
// but rendering should remain local.
// Use this function in update logic; keep the skipRender vtable entry unchanged
// so the entity only renders locally.
// References to skipRender in update should use this function instead.
bool ActorFixes_isInRangeOfAllPlayers(StageEntity* self)
{
	FxRect boundingBox;

	if (self->forceRender)
		return false;

	if (self->updateStateID == StageEntity::UpdateStateID::Carried)
		return false;

	boundingBox.x = self->viewOffset.x << 12;
	boundingBox.y = self->viewOffset.y << 12;
	boundingBox.halfWidth = self->renderSize.x << 11;
	boundingBox.halfHeight = self->renderSize.y << 11;

	return ActorFixes_isOutsideCamera(self, boundingBox);
}

// Hammer/Fire/Boomerang Bros -----------------------------------------------------------

ncp_over(0x0216E1D4, 54) const auto HammerBroSpawnPoint_skipRender = ActorFixes_safeSkipRender;

ncp_over(0x021754F8, 56) const auto HammerBro_skipRender = ActorFixes_safeSkipRender;
ncp_over(0x02175730, 56) const auto FireBro_skipRender = ActorFixes_safeSkipRender;
ncp_over(0x02175614, 56) const auto BoomerangBro_skipRender = ActorFixes_safeSkipRender;

// Sledge Bro -----------------------------------------------------------

ncp_over(0x02175880, 56) const auto SledgeBro_skipRender = ActorFixes_safeSkipRender;

NTR_USED static void SledgeBro_fixShakePlayer(StageEntity* self)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);

		if (ActorFixes_isPlayerInShakeRange(self, player))
		{
			ViewShaker::start(3, self->viewID, playerID, false);
			if (playerID == Game::localPlayerID)
				SND::playSFX(138, &self->position);
			if (!Game::getPlayerDead(player->linkedPlayerID))
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

// Restore multiplayer lava rising logic
ncp_repl(0x02165170, 54, "CMP R2, #1")
ncp_repl(0x02165274, 54, ".int _ZN4Game11playerCountE")

// Do not use Game::localPlayerID for lava rising
asm(R"(
ncp_over(0x021651B4, 54)
	LDRB    R2, [R2]
	LDR     R3, [R3]
ncp_endover()
)");

// TODO: LIQUID POSITION AND LAST LIQUID POSITION

// Boo ----------------------------------------------------------------------------------

ncp_over(0x021793EC, 71) const auto Boo_skipRender = ActorFixes_safeSkipRender;

// Balloon Boo --------------------------------------------------------------------------

ncp_over(0x02179508, 71) const auto BalloonBoo_skipRender = ActorFixes_safeSkipRender;

ncp_repl(0x0217716C, 71, "NOP") // Pass Boo* instead of &Boo*->position
ncp_set_call(0x02177178, 71, ActorFixes_isOutsideCamera)

// Rotating Carry Through Wall Platform -------------------------------------------------

ncp_over(0x0218FF54, 118) const auto RotatingCarryThroughWallPlatform_skipRender = ActorFixes_safeSkipRender;

// Blockhopper --------------------------------------------------------------------------

ncp_over(0x021784B8, 69) const auto Blockhopper_skipRender = ActorFixes_safeSkipRender;

asm(R"(
ncp_jump(0x02177260, 69)
	MOV     R0, R4
	BL      _Z27ActorFixes_getClosestPlayerP10StageActor
	B       0x02177264

ncp_over(0x021778A4, 69)
	NOP
	MOV     R0, R4
	BL      _Z27ActorFixes_getClosestPlayerP10StageActor
ncp_endover()
)");

ncp_set_call(0x02177450, 69, ActorFixes_getClosestPlayer)

// Since there is no "bahp" event in coop, use random jump intervals.
// Return value lower or equal to 0 means "do not jump".
NTR_USED static u32 Blockhoppper_isTimeToJump(StageEntity* self)
{
	if (Game::getPlayerCount() == 1)
	{
		return *rcast<u32*>(0x02088B9C); // bahp
	}
	return (Net::getRandom() & 0xFF) == 0;
}

ncp_repl(0x02177384, 69, "MOV R0, R4")

asm(R"(
ncp_jump(0x02177388, 69)
	PUSH    {R1}
	BL      _ZL25Blockhoppper_isTimeToJumpP11StageEntity
	POP     {R1}
	B       0x0217738C
)");

// Flying Red Block ---------------------------------------------------------------------

asm(R"(
ncp_over(0x0215BBE4, 54)
	MOV     R4, R0
	BL      _Z27ActorFixes_getClosestPlayerP10StageActor
	NOP
	NOP
ncp_endover()

ncp_over(0x0215BC48, 54)
	MOV     R4, R0
	BL      _Z27ActorFixes_getClosestPlayerP10StageActor
	NOP
	NOP
ncp_endover()

ncp_over(0x0215BCCC, 54)
	MOV     R0, R4
	BL      _Z27ActorFixes_getClosestPlayerP10StageActor
	NOP
ncp_endover()
)");

// Warp Cannon ---------------------------------------------------------------------

ncp_over(0x0217FD74, 89) const auto WarpCannon_skipRender = ActorFixes_safeSkipRender;

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

// Lakitu Spawner -----------------------------------------------------------------------

bool LakituSpawner_customTargetAvailable(LakituSpawner* self, Player* player)
{
	return !Game::getPlayerDead(player->linkedPlayerID) && self->targetAvailable(player);
}

ncp_set_call(0x02175BD8, 66, LakituSpawner_customTargetAvailable)
ncp_set_call(0x02175C4C, 66, LakituSpawner_customTargetAvailable)
ncp_set_call(0x02175C6C, 66, LakituSpawner_customTargetAvailable)
ncp_set_call(0x02175D0C, 66, LakituSpawner_customTargetAvailable)
ncp_set_call(0x02175D40, 66, LakituSpawner_customTargetAvailable)

// Warp Entrance ------------------------------------------------------------------------

// Fix the warp entrance not working after one usage

void WarpEntrance_customWarpPlayer(WarpEntrance* self, Player* player)
{
	self->warpPlayer(player);

	self->warpCountdown = 0;
	self->warpState = WarpEntrance::WarpState::None;
	self->warpTriggered = 0;
}

ncp_set_call(0x02156258, 54, WarpEntrance_customWarpPlayer)
ncp_set_call(0x02156350, 54, WarpEntrance_customWarpPlayer)

// Disable warp entrance if someone reached flagpole

bool WarpEntrance_customMainState(WarpEntrance* self)
{
	if (Stage_flagpoleLinkedPlayer != nullptr)
		return true;

	return self->mainState();
}

ncp_over(0x0216D27C, 54) const auto WarpEntrance_mainState = WarpEntrance_customMainState;

// Bullet Bill --------------------------------------------------------------------------

asm(R"(
ncp_jump(0x021470E8, 42)
	MOV     R0, R5
	BL      _Z27ActorFixes_getClosestPlayerP10StageActor
	B       0x021470EC
)");

// Sushi (Shark) ------------------------------------------------------------------------

ncp_set_call(0x02179E40, 78, ActorFixes_getClosestPlayer)

// Random Cheep Cheep Generator ---------------------------------------------------------

asm(R"(
ncp_jump(0x0213CBC8, 25)
	MOV     R0, R5
	BL      _Z27ActorFixes_getClosestPlayerP10StageActor
	B       0x0213CBCC
)");

// Coin ---------------------------------------------------------------------------------

asm(R"(
ncp_jump(0x020D89A8, 10)
	MOV     R0, R5
	BL      _Z27ActorFixes_getClosestPlayerP10StageActor
	B       0x020D89AC

ncp_jump(0x020D9D48, 10)
	MOV     R0, R4
	BL      _Z27ActorFixes_getClosestPlayerP10StageActor
	B       0x020D9D4C
)");

ncp_repl(0x020D98DC, 10, "MOV R0, #1") // Allow coins to get killed by lava

ncp_set_call(0x020D8524, 10, ActorFixes_isInRangeOfAllPlayers) // Fix coin permanent deletion

// Broozer ------------------------------------------------------------------------------

ncp_repl(0x0218A898, 108, "NOP") // Pass Broozer* instead of &Broozer*->position
ncp_set_call(0x0218A8A4, 108, ActorFixes_isOutsideCamera)

// Horizontal Camera Stop ---------------------------------------------------------------

u32 HorizontalCameraStop_playerID;

asm(R"(
.type HorizontalCameraStop_onCreate_SUPER, %function
HorizontalCameraStop_onCreate_SUPER:
	PUSH    {LR}
	B       0x020D6604
)");
extern "C" {
	s32 HorizontalCameraStop_onCreate_SUPER(StageEntity* self);
}

ncp_jump(0x020D6600, 10)
s32 HorizontalCameraStop_onCreate_OVERRIDE(StageEntity* self)
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		HorizontalCameraStop_playerID = playerID;
		HorizontalCameraStop_onCreate_SUPER(self);
	}

	return 0;
}

ncp_repl(0x020D6784, 10, ".int HorizontalCameraStop_playerID")

// Volcano Eruption ---------------------------------------------------------------------

// Original Desync Issues:
// 1. Meteor spawns were triggered by the background animation function. This function can fail to update
//    for the local player under certain conditions (e.g., camera freeze, cutscenes, or death), causing
//    volcano eruptions to desynchronize between consoles.
// 2. The usual fix for actor targeting (replacing Game::getLocalPlayer with the closest player) doesn't apply here.
//    In this case, the code was already using the closest player, but this is still unreliable because the
//    volcano eruption actor's position is relative to the local camera. Each console sees the actor in a
//    different place, so only meteors spawned with the player pointer are synchronized; the actor itself is not.
//
// Fixes:
// - The eruption logic is now handled after StageLayout::onUpdate (see ActorFixes_updateVolcanoBackground).
//   This ensures the eruption timer is updated consistently for all players.
// - Instead of targeting the closest player (which is unreliable due to local-only actor positions),
//   the code cycles through all players in order, so no one feels unfairly targeted and all consoles
//   spawn eruptions in sync.
//
// Additional changes:
// - Disabled the original background-triggered eruption and screen shake code with NOPs.
// - Patched the volcano eruption actor to allow out-of-range spawns, since range checks are not reliable
//   when actor positions are local-only.
// - Patched the actor's player targeting to use the round-robin method described above.

ncp_repl(0x020B6D2C, 0, "NOP") // Disable original volcano eruption trigger
ncp_repl(0x020B6D44, 0, "NOP") // Disable original volcano eruption trigger
ncp_repl(0x020B6D6C, 0, "NOP") // Disable original screen shake

asm(R"(
StageLayout_volcanoShake = 0x020AD65C
)");
extern "C" {
	void StageLayout_volcanoShake(StageLayout* self);
}

u16 ActorFixes_volcanoTimer = 60 * 8 - 1;
u8 ActorFixes_volcanoTargetPlayer = 0;

static inline u16 ActorFixes_getFgScreenID(u32 playerID)
{
	return *rcast<u16*>(&rcast<u8*>(Stage::stageLayout)[12 * playerID + 1196]);
}

struct bgScrollData
{
    fx32 field_0;
    fx32 bottomScrollF32;
    fx32 cameraOffset;
    fx32 topScrollF32;
};

void ActorFixes_updateVolcanoBackground()
{
	u32 playerID = Game::localPlayerID;

	u32& levelEndBitmask = *rcast<u32*>(0x020CA8C0);
	if ((levelEndBitmask & 3) != 0 || ActorFixes_getFgScreenID(playerID) != 15)
		return;

	ActorFixes_volcanoTimer++;
	if (ActorFixes_volcanoTimer != ActorFixes_volcanoTimerInterval)
		return;
	ActorFixes_volcanoTimer = 0;

	u8 viewID = Game::getPlayer(playerID)->viewID;

	bgScrollData* gStageHorizontalScrollData = rcast<bgScrollData*>(0x020CAF18);
	bgScrollData* gStageVerticalScrollData = rcast<bgScrollData*>(0x020CAF38);

	fx32 horizScroll = gStageHorizontalScrollData[playerID].topScrollF32 & 0xFF000;
	fx32 vertScroll  = gStageVerticalScrollData[playerID].topScrollF32 & 0x1FF000;
	fx32 horizOffset = gStageHorizontalScrollData[playerID].cameraOffset;
	fx32 vertOffset  = gStageVerticalScrollData[playerID].cameraOffset;

	// Compute eruption position
	Vec3 eruptionPos;
	eruptionPos.x = (0x2A000 - horizScroll) + horizOffset;
	eruptionPos.y = -(0x150000 - vertScroll + vertOffset);
	eruptionPos.z = 0;

	// First eruption
	VolcanoEruption::erupt(eruptionPos, viewID);

	// Second eruption, shifted right
	eruptionPos.x += 0x100000;
	VolcanoEruption::erupt(eruptionPos, viewID);

	ActorFixes_volcanoTargetPlayer++;
	if (ActorFixes_volcanoTargetPlayer == Game::getPlayerCount())
		ActorFixes_volcanoTargetPlayer = 0;

	StageLayout_volcanoShake(Stage::stageLayout);
}

// We cannot use the closest player for targeting, since the actor's position is local-only.
// Instead, cycle through all players in order to distribute eruptions fairly.
ncp_call(0x021633DC, 54)
Player* VolcanoEruption_fixGetClosestPlayerOnCreate()
{
	return Game::getPlayer(ActorFixes_volcanoTargetPlayer);
}

// Allow volcano eruptions to spawn even if the player is out of range,
// since range checks are unreliable with local-only actor positions.
ncp_repl(0x02162F2C, 54, "MOV R0, #0")

// Misc ---------------------------------------------------------------------------------

ncp_over(0x02132560, 18) const auto CheepSkipper_skipRender = ActorFixes_safeSkipRender;

ncp_over(0x0216D1BC, 54) const auto Trampoline_skipRender = ActorFixes_safeSkipRender;

ncp_over(0x0213F470, 24) const auto DryBones_skipRender = ActorFixes_safeSkipRender;

ncp_over(0x0218F450, 123) const auto Toadsworth_skipRender = ActorFixes_safeSkipRender;


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

// Tile God updates during stage freeze
ncp_repl(0x0216D3E4, 54, ".int _ZN10StageActor9preUpdateEv");


// TODO: check ov54:0215DEEC

// TODO: CHECK WHAT 020D98DC IS
