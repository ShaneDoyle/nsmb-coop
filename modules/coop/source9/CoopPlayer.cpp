#include "coop/CoopPlayer.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/game/stage/player/player.hpp>
#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/sound.hpp>
#include <nsmb/core/system/function.hpp>
#include <nsmb/core/net.hpp>

#include "coop/CoopStage.hpp"
#include "coop/CoopCamera.hpp"

#define PLAYER_JUMPED_ON_ANIM_STATE_WAIT 0xFF

extern "C" {
	void CoopSym_Liquid_doWaves(fx32 x, u32 one);
}

namespace CoopPlayer {

u8 jumpedOnAnimState[2];
u8 seqArcIDs[] = { 4, 30 };

void updateJumpedOnAnimation(Player* self)
{
	constexpr u32 GrowFrames = 8;
	constexpr fx32 HeightReduction = 0x400;

	u8& animState = jumpedOnAnimState[self->linkedPlayerID];
	if (animState == PLAYER_JUMPED_ON_ANIM_STATE_WAIT)
		return;

	// Cancel the animation if player is switching powerup
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

	fx32 scaledHeightReduction = FX_Div(FX_Mul(HeightReduction, self->modelScale), Player::constantsSmall.scale);
	self->scale.y = (scaledHeightReduction / GrowFrames) * animState + (originalScale - scaledHeightReduction);

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

void beginJumpedOnAnimation(Player* self)
{
	jumpedOnAnimState[self->linkedPlayerID] = 0;
	updateJumpedOnAnimation(self);
}

ncp_repl(0x02109B30, 10, "B 0x02109B84") // Mario can not make Luigi fall on head jump
ncp_repl(0x02109A14, 10, "B 0x02109A68") // Luigi can not make Mario fall on head jump

ncp_repl(0x02109EB4, 10, "MOV R4, #1") // Mario doesn't bump with Luigi
ncp_repl(0x02109C1C, 10, "MOV R4, #1") // Luigi doesn't bump with Mario

ncp_thumb bool customJumpOnPlayer(Player* self, fx32 force, u16 duration, bool playSFX, bool noConsecutive, s8 variation)
{
	if (self->physicsFlag.swimming)
		return false;

	if (self->doJump(force, duration, playSFX, noConsecutive, variation))
	{
		Player* other = Game::getPlayer(self->linkedPlayerID ^ 1);
		beginJumpedOnAnimation(other);
		return true;
	}

	return false;
}

ncp_set_call(0x02109AB8, 10, customJumpOnPlayer)
ncp_set_call(0x02109BD4, 10, customJumpOnPlayer)

ncp_thumb void customJumpOnPlayerSound(Player* self, s32 sfxID, const Vec3* pos)
{
	u8 entryID = 7 + (Net::getRandom() & 1);
	s32 otherID = self->linkedPlayerID ^ 1;
	SND::playSFX(seqArcIDs[otherID], entryID, pos, 127, SND::Internal::SFXMode::Unique);
}

ncp_set_call(0x02109AD0, 10, customJumpOnPlayerSound)
ncp_set_call(0x02109BEC, 10, customJumpOnPlayerSound)

ncp_repl(0x0204ED14, "NOP") // Always load both players' sounds
ncp_repl(0x0204ED50, "NOP") // Always load both players' sounds

ncp_call(0x021098C8, 10)
ncp_thumb bool customSpecialPlayerBump(Player* self, Player* other, fx32& selfCollisionPointX)
{
	bool marioOffender = Player::bumpOffender == Player::BumpOffender::Mario;
	Player* offender = marioOffender ? self : other;
	Player* victim = marioOffender ? other : self;

	if (offender->checkGroundpoundBump())
	{
		u32 timer = Game::getStarmanTimer(offender->linkedPlayerID);
		if (timer)
		{
			victim->applyStarman(timer);
		}

		s32 direction = StageEntity::unitDirection[(selfCollisionPointX < 0) ^ marioOffender];
		Vec2 velocity(0xD00 * direction, 0x3000);
		victim->doPlayerBump(velocity, true);
		return true;
	}

	return false;
}

bool ignoreCollision()
{
	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		Player* player = Game::getPlayer(playerID);
		if (isOnFlagpole(player) || player->currentPowerup == PowerupState::Mega || CoopStage::hasLevelFinished())
			return true;
	}
	return false;
}

ncp_asmfunc void ignoreCollisionPatch_ASM()
{asm(R"(
ncp_jump(0x021096EC, 10)
	BNE     0x021096F0
	MOV     R0, R9
	MOV     R1, R8
	BL      _ZN10CoopPlayer15ignoreCollisionEv
	CMP     R0, #0
	BNE     0x021096F0
	B       0x02109704
)");}

// TODO: turn this into CoopPlayer::fireballPassthrough
ncp_repl(0x020E3260, 10, "MOV R0, R4") // Fireballs pass through player
//ncp_repl(0x020E32C4, 10, "ADD SP, SP, #0x10; POP {R4-R6,PC}") // Player immune to fireballs

ncp_call(0x020FD56C, 10)
bool updateHook(Player* self)
{
	updateJumpedOnAnimation(self);
	return self->updateCarryPartialAnimation(); // Keep replaced instruction
}

void resetHook(Player* self)
{
	jumpedOnAnimState[self->playerID] = PLAYER_JUMPED_ON_ANIM_STATE_WAIT;
}

ncp_asmfunc void resetHookPatch_ASM()
{asm(R"(
ncp_jump(0x0210024C, 10)
	MOV     R0, R4
	BL      _ZN10CoopPlayer9resetHookEP6Player
	ADD     SP, SP, #8
	POP     {R4-R6,PC}
)");}

// Look at bosses

u8 isLookingAtTarget[2];

ncp_call(0x020FD6C4, 10)
bool updateLookAtHeadRotation_AT_020FD6C4_CALL(Player* player)
{
	return isLookingAtTarget[player->linkedPlayerID];
}

ncp_repl(0x020FD828, 10, "MOV R0, R4")

ncp_call(0x020FD82C, 10)
void updateLookAtHeadRotation_AT_020FD82C_CALL(Player* player)
{
	isLookingAtTarget[player->linkedPlayerID] = false;
}

ncp_jump(0x0202002C)
void Game_setPlayerLookingAtTarget_OVERRIDE(bool enable)
{
	isLookingAtTarget[0] = enable;
	isLookingAtTarget[1] = enable;
}

// Flinch at bosses

u8 isFlinching[2];

bool bossCutsceneTransitState_customIsPlayerFlinching(Player* player)
{
	return isFlinching[player->linkedPlayerID];
}

ncp_asmfunc void bossFlinchPatch_ASM()
{asm(R"(
ncp_jump(0x0211ACF4, 10)
	MOV     R0, R4
	BL      _ZN10CoopPlayer48bossCutsceneTransitState_customIsPlayerFlinchingEP6Player
	B       0x0211ACF8
)");}

ncp_repl(0x0211AD00, 10, "MOV R0, R4")

ncp_call(0x0211AD04, 10)
void bossCutsceneTransitState_AT_0211AD04_CALL(Player* player)
{
	isFlinching[player->linkedPlayerID] = false;
}

ncp_jump(0x0202000C)
void Game_setPlayerFlinching_OVERRIDE(bool enable)
{
	isFlinching[0] = enable;
	isFlinching[1] = enable;
}

// Boss defeat cutscene

Player* victoryLinkedPlayer = nullptr;
Vec3 victoryCutsceneStartPos;
u8 victoryIsBattleSwitch = false;
u8 victoryFakePlayerDeathTimer[2];

// For the players that didn't hit the switch
ncp_thumb bool bossDefeatNotLinkedTransitState(Player* self, void* arg)
{
	const u32 FakeDeathDuration = 120;

	const u32 STEP_WaitVictoryPose = 1;
	const u32 STEP_WaitWallBreak = 2;
	const u32 STEP_WaitGroundLand = 3;
	const u32 STEP_WaitPeachReact = 4;
	const u32 STEP_MissedCutscene = 5;

	s32 playerID = self->linkedPlayerID;

	s8& step = self->transitionStateStep;
	u8& deathTimer = victoryFakePlayerDeathTimer[playerID];

	if (step == Func::Init)
	{
		self->velocity.x = 0;
		self->velocity.y = 0;
		deathTimer = 0;

		bool grounded = (scast<u32>(self->collisionMgr.bottomResult) & CollisionMgr::Result::GroundAny);
		if (!grounded)
		{
			// Fall and try to join in on the animation later
			self->setAnimation(6, true, Player::FrameMode::Restart, 1fx);
			self->subActionFlag.releaseKeys = true;

			step = STEP_WaitGroundLand;
		}
		else
		{
			step = STEP_WaitVictoryPose;
		}

		return true;
	}
	if (step == Func::Exit)
	{
		return true;
	}

	auto victoryDancing = [&]{ return victoryLinkedPlayer->transitionStateStep >= 3; };
	auto wallsBreaking = [&]{ return victoryLinkedPlayer->transitionStateStep >= 6; };
	auto peachReacting = [&]{ return victoryLinkedPlayer->transitionStateStep >= 10; };

	if (step == STEP_WaitVictoryPose)
	{
		// Wait for player that hit the button to start animation
		if (victoryDancing())
		{
			// Begin victory pose
			self->rotation.y = 0;
			self->bossBeginVictoryPose();

			step = STEP_WaitWallBreak;
		}
	}
	else if (step == STEP_WaitWallBreak)
	{
		// Wait for barrier blocks to start breaking
		if (wallsBreaking())
		{
			if (!victoryIsBattleSwitch ||
				Math::abs(victoryLinkedPlayer->position.x - self->position.x) < (16fx * 4fx))
			{
				// Player is with the one that hit the switch
				self->switchTransitionState(&Player::bossVictoryTransitState);
				step = 5; // Skip some steps in the state we just switched to
			}
			else
			{
				// Player is not near the one that hit the switch
				self->setAnimation(0, true, Player::FrameMode::Restart, 1fx);

				step = STEP_WaitPeachReact;
			}

			CoopCamera::setLerping(playerID, true);
			CoopCamera::setFollowTarget(playerID, victoryLinkedPlayer->linkedPlayerID);
		}
	}
	else if (step == STEP_WaitGroundLand)
	{
		self->updateGravityAcceleration();
		self->updateVerticalVelocityClamped();
		self->applyVelocity();

		bool grounded = (scast<u32>(self->collisionMgr.updatePlayerGroundCollision()) & CollisionMgr::Result::GroundAny);

		if (grounded)
		{
			if (wallsBreaking())
			{
				self->setAnimation(0, true, Player::FrameMode::Restart, 1fx);
				step = STEP_WaitWallBreak;
			}
			else
			{
				step = STEP_WaitVictoryPose;
			}
		}
		else
		{
			// If barrier blocks started breaking and we haven't landed yet
			if (wallsBreaking())
			{
				// Then the player missed the cutscene
				CoopCamera::setLerping(playerID, true);
				CoopCamera::setFollowTarget(playerID, victoryLinkedPlayer->linkedPlayerID);

				step = STEP_MissedCutscene;
			}
		}
	}
	else if (step == STEP_WaitPeachReact)
	{
		// Wait for Peach react animation
		if (peachReacting())
		{
			self->position = victoryCutsceneStartPos;

			self->switchTransitionState(&Player::bossVictoryTransitState);
			step = 10; // Skip some steps in the state we just switched to
		}
	}

	// Custom lava death
	if (deathTimer < FakeDeathDuration)
	{
		if (Game::getPlayerDead(playerID))
		{
			deathTimer++;
		}
		else if (self->position.y < Stage::liquidPosition[Game::localPlayerID])
		{
			self->playSFXUnique(338, &self->position);
			CoopSym_Liquid_doWaves(self->position.x, 1);
			Game::losePlayerLife(playerID);
			Game::setPlayerDead(playerID, true);
		}
	}
	else if (deathTimer == FakeDeathDuration)
	{
		CoopStage::isPlayerDead[playerID] = true;
		deathTimer++;
	}

	self->updateAnimation();

	return true;
}

void beginBossDefeatCutsceneNotLinked(Player* self)
{
	self->switchMainState(&Player::idleState);
	self->switchTransitionState(ptmf_cast(bossDefeatNotLinkedTransitState));
}

ncp_thumb void beginBossDefeatCutsceneCoop(Player* linkedPlayer, bool battleSwitch)
{
	victoryLinkedPlayer = linkedPlayer;
	victoryCutsceneStartPos = linkedPlayer->position;
	victoryIsBattleSwitch = battleSwitch;

	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		if (playerID == linkedPlayer->linkedPlayerID || Game::getPlayerDead(playerID))
			continue;

		Player* player = Game::getPlayer(playerID);
		beginBossDefeatCutsceneNotLinked(player);
	}
}

ncp_call(0x0211881C, 10)
ncp_thumb u32 viewTransitState_beginFadeInHook(u8 transitPlayerID)
{
	if (Game::getPlayerDead(transitPlayerID))
		goto commonEnd;

	if (CoopStage::areaHasRotator())
	{
		// If a level has a rotator, all players must always be in the same view

		for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
		{
			if (playerID == transitPlayerID)
				continue;

			if (Game::getPlayerDead(playerID))
			{
				CoopCamera::followTargetToNewView(playerID, transitPlayerID);
				continue;
			}

			Entrance::setSpawnEntrance(Entrance::targetEntranceID, playerID);

			// Use entrance
			Player* player = Game::getPlayer(playerID);
			player->switchTransitionState(&Player::viewTransitState);
		}

		goto commonEnd;
	}

	CoopCamera::syncSpectatorsOnViewTransition(transitPlayerID);

commonEnd:
	return Entrance::getSpawnMusic(transitPlayerID); // Keep replaced instruction
}

// The invincible flag is typically set only for the local player,
// which should be fine since it seems to only control music playback.
// However, to ensure compatibility with ROM hacks or any singleplayer
// code that might rely on this flag, update it for all players.

/*
ncp_jump(0x0212B740, 11)
void fixInvincibleFlag(PlayerBase* self, s32 bgmID)
{
	self->subActionFlag.invincible = true;
	self->invincibleMusicID = bgmID;

	if (self->linkedPlayerID == Game::localPlayerID)
	{
		SND::requestSpecialBGM(bgmID);
	}
}
*/

ncp_over(0x0212B740, 11) /* max over: 0x7C bytes, current: 0x54 bytes */
ncp_asmfunc void fixInvincibleFlag_ASM()
{asm(R"(
	mov	r3, r0
	ldrb	r2, [r3, #1918]
	str	r1, [r3, #1944]
	orr	r2, r2, #128
	strb	r2, [r3, #1918]
	add	r3, r3, #284
	ldrsb	r2, [r3, #2]
	ldr	r3, .fixinv_L147
	mov	r0, r1
	ldr	r3, [r3]
	cmp	r2, r3
	bxne	lr
	b	_ZN3SND17requestSpecialBGMEl

ncp_jump(0x0212B730, 11)
	LDR     R3, .fixinv_L147
	LDR     R3, [R3]
	LDRB    R2, [R0,#0x11E]
	CMP     R3, R2
	LDREQ   R0, [R0,#0x798]
	BLEQ    _ZN3SND16stopRequestedBGMEl
	B       0x0212B738

.fixinv_L147:
	.word	_ZN4Game13localPlayerIDE
)");}

// Allow the game to wait for the player to shrink without the freeze flag

bool customUpdatePowerupStateMegaShrinking(::Player* self)
{
	bool ret = self->updatePowerupState();

	if (Game::playerCount != 1)
		ret = self->transitionFlag.megaShrink;

	return ret;
}

ncp_set_call(0x0211989C, 10, customUpdatePowerupStateMegaShrinking)
ncp_set_call(0x02119A10, 10, customUpdatePowerupStateMegaShrinking)
ncp_set_call(0x0211A528, 10, customUpdatePowerupStateMegaShrinking)
ncp_set_call(0x0211B0A0, 10, customUpdatePowerupStateMegaShrinking)
ncp_set_call(0x0211BB84, 10, customUpdatePowerupStateMegaShrinking)

// asm(R"(
// PlayerBase_freezeStage_SUPER:
// 	PUSH    {LR}
// 	B       0x0212C134
// )");
// extern "C" {
// 	void PlayerBase_freezeStage_SUPER(PlayerBase* self);
// }

// ncp_jump(0x0212C130, 11)
// void PlayerBase_freezeStage_OVERRIDE(PlayerBase* self)
// {
// 	if (Game::getPlayerCount() != 1)
// 		return;
// 	PlayerBase_freezeStage_SUPER(self);
// }

} // namespace CoopPlayer
