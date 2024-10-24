#include "nsmb/stage/player/player.hpp"
#include "nsmb/stage/entity.hpp"
#include "nsmb/sound.hpp"

#include "Stage.hpp"

#define PLAYER_JUMPED_ON_ANIM_STATE_WAIT 0xFF

static u8 Player_jumpedOnAnimState[2];

static void Player_updateJumpedOnAnimation(Player* self)
{
	constexpr u32 GrowFrames = 8;
	constexpr fx32 MaxHeightReduction = 0x400;

	u8& animState = Player_jumpedOnAnimState[self->linkedPlayerID];
	if (animState == PLAYER_JUMPED_ON_ANIM_STATE_WAIT)
		return;

	// Cancel the animation if player has is switching powerup
	if (self->powerupSwitchTimer != 0)
	{
		animState = PLAYER_JUMPED_ON_ANIM_STATE_WAIT;
		return;
	}

	fx32 originalScale = FX_Mul(self->modelScale, Player::modelScaleFactor);

	if (animState == GrowFrames)
	{
		self->scale.y = originalScale;
		animState = PLAYER_JUMPED_ON_ANIM_STATE_WAIT;
		return;
	}

	self->scale.y = (MaxHeightReduction / GrowFrames) * animState + (originalScale - MaxHeightReduction);

	animState++;

	/*
	constexpr u32 ShrinkFrames = 1;

	if (animState < ShrinkFrames)
	{
		self->scale.y = (MaxHeightReduction / ShrinkFrames) * (ShrinkFrames - animState) + (originalScale - MaxHeightReduction);
	}
	else if (animState < ShrinkFrames + GrowFrames)
	{
		self->scale.y = (MaxHeightReduction / GrowFrames) * (animState - ShrinkFrames) + (originalScale - MaxHeightReduction);
	}

	animState++;
	if (animState == ShrinkFrames + GrowFrames)
	{
		self->scale.y = originalScale;
		animState = PLAYER_JUMPED_ON_ANIM_STATE_WAIT;
	}
	*/
}

static void Player_beginJumpedOnAnimation(Player* self)
{
	Player_jumpedOnAnimState[self->linkedPlayerID] = 0;
	Player_updateJumpedOnAnimation(self);
}

ncp_repl(0x02109B30, 10, "B 0x02109B84") // Mario can not make Luigi fall on head jump
ncp_repl(0x02109A14, 10, "B 0x02109A68") // Luigi can not make Mario fall on head jump

ncp_repl(0x02109EB4, 10, "MOV R4, #1") // Mario doesn't bump with Luigi
ncp_repl(0x02109C1C, 10, "MOV R4, #1") // Luigi doesn't bump with Mario

static bool Player_customJumpOnPlayer(Player* self, fx32 force, u16 duration, bool playSFX, bool noConsecutive, s8 variation)
{
	if (self->physicsFlag.swimming)
		return false;

	if (self->doJump(force, duration, playSFX, noConsecutive, variation))
	{
		Player* other = Game::getPlayer(self->linkedPlayerID ^ 1);
		Player_beginJumpedOnAnimation(other);
		return true;
	}

	return false;
}

ncp_set_call(0x02109AB8, 10, Player_customJumpOnPlayer)
ncp_set_call(0x02109BD4, 10, Player_customJumpOnPlayer)

ncp_call(0x021098C8, 10)
static bool Player_customSpecialPlayerBump(Player* self, Player* other, fx32& selfCollisionPointX)
{
	bool marioOffender = Player::bumpOffender == Player::BumpOffender::Mario;
	Player* offender = marioOffender ? self : other;
	Player* victim = marioOffender ? other : self;

	if (offender->checkGroundpoundBump())
	{
		s32 direction = StageEntity::unitDirection[(selfCollisionPointX < 0) ^ marioOffender];
		Vec2 velocity(0xD00 * direction, 0x3000);
		victim->doPlayerBump(velocity, true);
		return true;
	}

	return false;
}

NTR_USED static bool Player_ignoreCollision()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);
		if (player->currentPowerup == PowerupState::Mega || Stage_hasLevelFinished())
			return true;
	}
	return false;
}

asm(R"(
ncp_jump(0x021096EC, 10)
	BNE     0x021096F0
	MOV     R0, R9
	MOV     R1, R8
	BL      _ZL22Player_ignoreCollisionv
	CMP     R0, #0
	BNE     0x021096F0
	B       0x02109704
)");

ncp_repl(0x020E3260, 10, "MOV R0, R4") // Fireballs pass through player
//ncp_repl(0x020E32C4, 10, "ADD SP, SP, #0x10; POP {R4-R6,PC}") // Player immune to fireballs

ncp_call(0x020FD56C, 10)
static bool Player_updateHook(Player* self)
{
	Player_updateJumpedOnAnimation(self);
	return self->updateCarryPartialAnimation(); // Keep replaced instruction
}

NTR_USED static void Player_resetHook(Player* self)
{
	Player_jumpedOnAnimState[self->playerID] = PLAYER_JUMPED_ON_ANIM_STATE_WAIT;
}

asm(R"(
ncp_jump(0x0210024C, 10)
	MOV     R0, R4
	BL      _ZL16Player_resetHookP6Player
	ADD     SP, SP, #8
	POP     {R4-R6,PC}
)");
