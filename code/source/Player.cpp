#include <nsmb/game/game.hpp>
#include <nsmb/game/stage/player/player.hpp>
#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/sound.hpp>
#include <nsmb/core/system/function.hpp>
#include <nsmb/core/net.hpp>

#include "Stage.hpp"
#include "PlayerSpectate.hpp"

#define PLAYER_JUMPED_ON_ANIM_STATE_WAIT 0xFF

static u8 Player_jumpedOnAnimState[2];
static u8 Player_seqArcIDs[] = { 4, 30 };


bool Player_isOnFlagpole(Player* self)
{
	return self->actionFlag.flagpoleGrab;
}

static void Player_updateJumpedOnAnimation(Player* self)
{
	constexpr u32 GrowFrames = 8;
	constexpr fx32 HeightReduction = 0x400;

	u8& animState = Player_jumpedOnAnimState[self->linkedPlayerID];
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

static void Player_customJumpOnPlayerSound(Player* self, s32 sfxID, const Vec3* pos)
{
	u8 entryID = 7 + (Net::getRandom() & 1);
	s32 otherID = self->linkedPlayerID ^ 1;
	SND::playSFX(Player_seqArcIDs[otherID], entryID, pos, 127, SND::Internal::SFXMode::Unique);
}

ncp_set_call(0x02109AD0, 10, Player_customJumpOnPlayerSound)
ncp_set_call(0x02109BEC, 10, Player_customJumpOnPlayerSound)

ncp_repl(0x0204ED14, "NOP") // Always load both players' sounds
ncp_repl(0x0204ED50, "NOP") // Always load both players' sounds

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
		if (Player_isOnFlagpole(player) || player->currentPowerup == PowerupState::Mega || Stage_hasLevelFinished())
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

// Look at bosses

static u8 sIsPlayerLookingAtTarget[2];

ncp_call(0x020FD6C4, 10)
bool Player_updateLookAtHeadRotation_AT_020FD6C4_CALL(Player* player)
{
	return sIsPlayerLookingAtTarget[player->linkedPlayerID];
}

ncp_repl(0x020FD828, 10, "MOV R0, R4")

ncp_call(0x020FD82C, 10)
void Player_updateLookAtHeadRotation_AT_020FD82C_CALL(Player* player)
{
	sIsPlayerLookingAtTarget[player->linkedPlayerID] = false;
}

ncp_jump(0x0202002C)
void Game_setPlayerLookingAtTarget_OVERRIDE(bool enable)
{
	sIsPlayerLookingAtTarget[0] = enable;
	sIsPlayerLookingAtTarget[1] = enable;
}

// Flinch at bosses

static u8 sIsPlayerFlinching[2];

bool Player_bossCutsceneTransitState_customIsPlayerFlinching(Player* player)
{
	return sIsPlayerFlinching[player->linkedPlayerID];
}

asm(R"(
ncp_jump(0x0211ACF4, 10)
	MOV     R0, R4
	BL      _Z55Player_bossCutsceneTransitState_customIsPlayerFlinchingP6Player
	B       0x0211ACF8
)");

ncp_repl(0x0211AD00, 10, "MOV R0, R4")

ncp_call(0x0211AD04, 10)
void Player_bossCutsceneTransitState_AT_0211AD04_CALL(Player* player)
{
	sIsPlayerFlinching[player->linkedPlayerID] = false;
}

ncp_jump(0x0202000C)
void Game_setPlayerFlinching_OVERRIDE(bool enable)
{
	sIsPlayerFlinching[0] = enable;
	sIsPlayerFlinching[1] = enable;
}

// Boss defeat cutscene

asm(R"(
	Liquid_doWaves = 0x021646E0
)");
extern "C" {
	void Liquid_doWaves(fx32 x, u32 one);
}

Player* Player_victoryLinkedPlayer = nullptr;
Vec3 Player_victoryCutsceneStartPos;
u8 Player_victoryIsBattleSwitch = false;
u8 Player_victoryFakePlayerDeathTimer[2];

// For the players that didn't hit the switch
bool Player_bossDefeatNotLinkedTransitState(Player* self, void* arg)
{
	const u32 FakeDeathDuration = 120;

	const u32 STEP_WaitVictoryPose = 1;
	const u32 STEP_WaitWallBreak = 2;
	const u32 STEP_WaitGroundLand = 3;
	const u32 STEP_WaitPeachReact = 4;
	const u32 STEP_MissedCutscene = 5;

	s32 playerID = self->linkedPlayerID;

	s8& step = self->transitionStateStep;
	u8& deathTimer = Player_victoryFakePlayerDeathTimer[playerID];

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

	auto victoryDancing = [&]{ return Player_victoryLinkedPlayer->transitionStateStep >= 3; };
	auto wallsBreaking = [&]{ return Player_victoryLinkedPlayer->transitionStateStep >= 6; };
	auto peachReacting = [&]{ return Player_victoryLinkedPlayer->transitionStateStep >= 10; };

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
			if (!Player_victoryIsBattleSwitch ||
				Math::abs(Player_victoryLinkedPlayer->position.x - self->position.x) < (16fx * 4fx))
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

			PlayerSpectate::setLerping(playerID, true);
			PlayerSpectate::setTarget(playerID, Player_victoryLinkedPlayer->linkedPlayerID);
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
				PlayerSpectate::setLerping(playerID, true);
				PlayerSpectate::setTarget(playerID, Player_victoryLinkedPlayer->linkedPlayerID);

				step = STEP_MissedCutscene;
			}
		}
	}
	else if (step == STEP_WaitPeachReact)
	{
		// Wait for Peach react animation
		if (peachReacting())
		{
			self->position = Player_victoryCutsceneStartPos;

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
			Liquid_doWaves(self->position.x, 1);
			Game::losePlayerLife(playerID);
			Game::setPlayerDead(playerID, true);
		}
	}
	else if (deathTimer == FakeDeathDuration)
	{
		Stage_isPlayerDead[playerID] = true;
		deathTimer++;
	}

	self->updateAnimation();

	return true;
}

void Player_beginBossDefeatCutsceneNotLinked(Player* self)
{
	self->switchMainState(&Player::idleState);
	self->switchTransitionState(ptmf_cast(Player_bossDefeatNotLinkedTransitState));
}

void Player_beginBossDefeatCutsceneCoop(Player* linkedPlayer, bool battleSwitch)
{
	Player_victoryLinkedPlayer = linkedPlayer;
	Player_victoryCutsceneStartPos = linkedPlayer->position;
	Player_victoryIsBattleSwitch = battleSwitch;

	for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		if (playerID == linkedPlayer->linkedPlayerID || Game::getPlayerDead(playerID))
			continue;

		Player* player = Game::getPlayer(playerID);
		Player_beginBossDefeatCutsceneNotLinked(player);
	}
}

ncp_call(0x0211881C, 10)
u32 Player_viewTransitState_beginFadeInHook(u8 transitPlayerID)
{
	if (Game::getPlayerDead(transitPlayerID))
		goto commonEnd;

	if (Stage_areaHasRotator())
	{
		// If a level has a rotator, all players must always be in the same view

		for (s32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
		{
			if (playerID == transitPlayerID)
				continue;

			if (Game::getPlayerDead(playerID))
			{
				PlayerSpectate::followTargetToNewView(playerID, transitPlayerID);
				continue;
			}

			Entrance::setSpawnEntrance(Entrance::targetEntranceID, playerID);

			// Use entrance
			Player* player = Game::getPlayer(playerID);
			player->switchTransitionState(&Player::viewTransitState);
		}

		goto commonEnd;
	}

	PlayerSpectate::syncSpectatorsOnViewTransition(transitPlayerID);

commonEnd:
	return Entrance::getSpawnMusic(transitPlayerID); // Keep replaced instruction
}

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
