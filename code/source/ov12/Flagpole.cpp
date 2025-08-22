#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/player.hpp>
#include <nsmb/core/system/function.hpp>
#include <nsmb/core/graphics/fader.hpp>
#include <nsmb/core/graphics/2d/oam.hpp>

#include "Stage.hpp"
#include "PlayerSpectate.hpp"

void Player_beginMissedPoleState(Player* self);
void Player_beginSentFlyingAwayWithPoleState(Player* self);

// Flagpole -----------------------------------------------------------------------------

constexpr u32 FlagpoleMaxPlayerCount = 2; // TODO: replace with global definition

struct Flagpole_PTMF
{
	bool (*func)(StageEntity*);
	u32 adj;
};

StageEntity* Flagpole_instance;
// Lowest player has value 0, second has 1, third has 2
u8 Flagpole_playerOrdinal[FlagpoleMaxPlayerCount];
u8 Flagpole_waitPlayerCountdown;
u8 Flagpole_scoreSprites[FlagpoleMaxPlayerCount];
u8 Flagpole_scoreSpriteRenderOrder[FlagpoleMaxPlayerCount];
u8 Flagpole_scoreSpriteRenderOffset[FlagpoleMaxPlayerCount];
u8 Flagpole_scoreSpriteCount;

extern "C" {
	void setSecretExitFromSpriteData(u32);
	bool Flagpole_switchState(StageEntity* self, Flagpole_PTMF* function);
	bool Flagpole_updateGoalGrab(StageEntity* self);
	extern Flagpole_PTMF Flagpole_sPlayerSlide;
}

ncp_asmfunc bool Player_flagpoleTransitState_SUPER(Player* self, void* arg)
{asm(R"(
	PUSH    {R4,LR}
	B       0x0211B5CC
)");}

void Flagpole_getPlayersGrabbing(u32* polePlayerCount, Player** polePlayers, u32* notPolePlayerCount, Player** notPolePlayers, bool* allGrabbing)
{
	bool _allGrabbing = true;
	u32 _polePlayerCount = 0;
	u32 _notPolePlayerCount = 0;

	for (u32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		if (Game::getPlayerDead(playerID))
			continue;

		Player* player = Game::getPlayer(playerID);
		if (!player->actionFlag.flagpoleGrab)
		{
			_allGrabbing = false;

			if (notPolePlayers)
				notPolePlayers[_notPolePlayerCount] = player;
			_notPolePlayerCount++;

			continue;
		}

		if (polePlayers)
			polePlayers[_polePlayerCount] = player;
		_polePlayerCount++;
	}

	if (polePlayerCount)
		*polePlayerCount = _polePlayerCount;
	if (notPolePlayerCount)
		*notPolePlayerCount = _notPolePlayerCount;
	if (allGrabbing)
		*allGrabbing = _allGrabbing;
}

void Flagpole_calculatePlayerOrdinals(u32 playerCount, Player** players)
{
	for (u32 i = 0; i < playerCount; i++)
	{
		u32 ordinal = 0;

		for (u32 j = 0; j < playerCount; j++)
		{
			if (i != j && players[j]->position.y < players[i]->position.y)
				ordinal++;
		}

		Flagpole_playerOrdinal[players[i]->linkedPlayerID] = ordinal;
	}
}

void Flagpole_beginLevelEnd()
{
	*rcast<u32*>(0x020CA8C0) |= 3; // levelEndBitmask
	rcast<Player*>(0)->stopBGM(32);
}

void Flagpole_switchToPlayerSlideState(StageEntity* self)
{
	u32 polePlayerCount;
	Player* polePlayers[FlagpoleMaxPlayerCount];
	u32 notPolePlayerCount;
	Player* notPolePlayers[FlagpoleMaxPlayerCount];
	Flagpole_getPlayersGrabbing(&polePlayerCount, polePlayers, &notPolePlayerCount, notPolePlayers, nullptr);

	Flagpole_calculatePlayerOrdinals(polePlayerCount, polePlayers);

	Flagpole_switchState(self, &Flagpole_sPlayerSlide);

	for (u32 i = 0; i < notPolePlayerCount; i++)
	{
		Player* player = notPolePlayers[i];
		Player_beginMissedPoleState(player);
	}

	Flagpole_beginLevelEnd();
}

void Flagpole_sendPlayersFlyingAway()
{
	u32 polePlayerCount;
	Player* polePlayers[FlagpoleMaxPlayerCount];
	Flagpole_getPlayersGrabbing(&polePlayerCount, polePlayers, nullptr, nullptr, nullptr);

	for (u32 i = 0; i < polePlayerCount; i++)
	{
		Player* player = polePlayers[i];
		if (player != Stage_flagpoleLinkedPlayer) // Because Mega breaking the pole counts as grabbing
			Player_beginSentFlyingAwayWithPoleState(player);
	}
}

bool Flagpole_allPlayersSlidingPole()
{
	u32 polePlayerCount;
	Player* polePlayers[FlagpoleMaxPlayerCount];
	Flagpole_getPlayersGrabbing(&polePlayerCount, polePlayers, nullptr, nullptr, nullptr);

	for (u32 i = 0; i < polePlayerCount; i++)
	{
		if (!polePlayers[i]->actionFlag.flagpoleSlide)
			return false;
	}

	return true;
}

void Flagpole_fixFinishSlide()
{
	u32 polePlayerCount;
	Player* polePlayers[FlagpoleMaxPlayerCount];
	Flagpole_getPlayersGrabbing(&polePlayerCount, polePlayers, nullptr, nullptr, nullptr);

	for (u32 i = 0; i < polePlayerCount; i++)
		polePlayers[i]->actionFlag.flagpoleEnd = true;
}

ncp_asmfunc void Flagpole_allPlayersSliding_ASM()
{asm(R"(
ncp_over(0x02130154, 12)
	BL      _Z30Flagpole_allPlayersSlidingPolev
	CMP     R0, #0
	BEQ     0x021301BC
	NOP
	NOP
	NOP
ncp_endover()

ncp_over(0x021301B0, 12)
	BL      _Z23Flagpole_fixFinishSlidev
	NOP
	NOP
ncp_endover()
)");}

ncp_repl(0x0213056C, 12, "NOP") // Only switch the state when everyone grabbed to keep the pole tangible

fx32 Flagpole_getPlayerHeight(Player* player, Player* playerAbove)
{
	fx32 height = player->scale.x * 16;

	// Make positioning more natural
	if (playerAbove != nullptr &&
		playerAbove->getHeight() == PlayerHeight::Super &&
		player->getHeight() != PlayerHeight::Mini)
	{
		height >>= 1; // Halve the height
	}

	return height;
}

fx32 Flagpole_getAccumulatedHeightsBelow(Player* targetPlayer)
{
	u32 polePlayerCount;
	Player* polePlayers[FlagpoleMaxPlayerCount];
	Flagpole_getPlayersGrabbing(&polePlayerCount, polePlayers, nullptr, nullptr, nullptr);

	fx32 accumulatedHeight = 0;
	u8 targetOrdinal = Flagpole_playerOrdinal[targetPlayer->linkedPlayerID];

	// Sum up heights of all players with lower ordinals (below the target player)
	for (u32 i = 0; i < polePlayerCount; i++)
	{
		Player* player = polePlayers[i];
		u8 playerOrdinal = Flagpole_playerOrdinal[player->linkedPlayerID];

		if (playerOrdinal < targetOrdinal)
		{
			// Find the player directly above this one
			Player* playerAbove = nullptr;
			u8 nextHigherOrdinal = playerOrdinal + 1;

			for (u32 j = 0; j < polePlayerCount; j++)
			{
				if (Flagpole_playerOrdinal[polePlayers[j]->linkedPlayerID] == nextHigherOrdinal)
				{
					playerAbove = polePlayers[j];
					break;
				}
			}

			accumulatedHeight += Flagpole_getPlayerHeight(player, playerAbove);
		}
	}

	return accumulatedHeight;
}

void Flagpole_adjustPlayerPositions(StageEntity* self, Player* grabber)
{
	// TODO: this function is not fully prepared for more than 2 players

	u32 polePlayerCount;
	Player* polePlayers[FlagpoleMaxPlayerCount];
	Flagpole_getPlayersGrabbing(&polePlayerCount, polePlayers, nullptr, nullptr, nullptr);

	// Find overlapping players and adjust positions
	for (u32 i = 0; i < polePlayerCount; i++)
	{
		Player* player = polePlayers[i];
		if (player == grabber)
			continue; // Skip the grabber

		u32 playerHeight = Flagpole_getPlayerHeight(player, grabber);
		u32 halfPlayerHeight = playerHeight >> 1;
		u32 grabberHeight = Flagpole_getPlayerHeight(grabber, nullptr);
		u32 halfGrabberHeight = grabberHeight >> 1;

		// Check if grabber overlaps with this player
		fx32 playerBottom = player->position.y;
		fx32 playerTop = player->position.y + playerHeight;
		fx32 grabberBottom = grabber->position.y;
		fx32 grabberTop = grabber->position.y + grabberHeight;

		// Check for vertical overlap
		if (!(grabberTop <= playerBottom || grabberBottom >= playerTop))
		{
			// There's an overlap, determine which half the grabber overlaps with
			fx32 playerMidpoint = playerBottom + halfPlayerHeight; // Middle of the player
			fx32 grabberMidpoint = grabberBottom + halfGrabberHeight; // Middle of the grabber

			if (grabberMidpoint < playerMidpoint)
			{
				// Grabber overlaps with bottom half, move grabber below the player
				grabber->position.y = playerBottom - grabberHeight;
			}
			else
			{
				// Grabber overlaps with top half, move grabber above the player
				grabber->position.y = playerTop;
			}
			break; // Only adjust for the first overlapping player found
		}
	}

	// Check if any player is now below the flagpole and shift all up if needed
	fx32 lowestPlayerY = self->position.y; // Start with flagpole position
	for (u32 i = 0; i < polePlayerCount; i++)
	{
		if (polePlayers[i]->position.y < lowestPlayerY)
			lowestPlayerY = polePlayers[i]->position.y;
	}

	// If any player is below the flagpole, shift all players up
	if (lowestPlayerY < self->position.y)
	{
		fx32 shiftAmount = self->position.y - lowestPlayerY;
		for (u32 i = 0; i < polePlayerCount; i++)
		{
			polePlayers[i]->position.y += shiftAmount;
		}
	}
}

void Flagpole_afterTouched(StageEntity* self)
{
	u16& grabberID = rcast<u16*>(self)[0x756 / 2];

	Player* grabber = Game::getPlayer(grabberID);

	if (grabber->currentPowerup == PowerupState::Mega)
	{
		Stage_flagpoleLinkedPlayer = grabber; // Mega player now owns the flagpole
		Flagpole_instance = self;

		*rcast<u8*>(0x020CA898) |= 0x40; // Stop time counter
		Flagpole_beginLevelEnd();

		Flagpole_sendPlayersFlyingAway();
		return;
	}

	if (Stage_flagpoleLinkedPlayer == nullptr) // Pole grabbed for the first time
	{
		Stage_flagpoleLinkedPlayer = grabber;
		Flagpole_waitPlayerCountdown = 180; // 3 seconds (same as NSMB Wii)
		Flagpole_instance = self;

		*rcast<u8*>(0x020CA898) |= 0x40; // Stop time counter
	}

	// Adjust player positions to prevent overlap whenever someone grabs the pole
	Flagpole_adjustPlayerPositions(self, grabber);

	// Check if everyone is grabbing the pole

	bool allGrabbing;
	Flagpole_getPlayersGrabbing(nullptr, nullptr, nullptr, nullptr, &allGrabbing);

	if (!allGrabbing)
		return;

	// Everyone is grabbing the pole, proceed with the pole slide

	Flagpole_waitPlayerCountdown = 0;
	Flagpole_switchToPlayerSlideState(self);
}

ncp_asmfunc void Flagpole_afterTouched_ASM()
{asm(R"(
ncp_over(0x02130570, 12)
	MOV     R0, R6
	BL      _Z21Flagpole_afterTouchedP11StageEntity
	B       0x021305B8
ncp_endover()
)");}

ncp_repl(0x02130588, 12, "NOP") // Do it in Flagpole_beginLevelEnd

ncp_repl(0x02130698, 12, "MOV R0, R6") // Pass Flagpole* instead of Flagpole*->settings

ncp_call(0x0213069C, 12)
void Flagpole_storeScoreSprite(StageEntity* self)
{
	setSecretExitFromSpriteData(self->settings); // Keep replaced instruction

	u32 polePlayerCount;
	Flagpole_getPlayersGrabbing(&polePlayerCount, nullptr, nullptr, nullptr, nullptr);

	u8 spriteIndex = rcast<u16*>(self)[0x754 / 2];
	Flagpole_scoreSprites[polePlayerCount - 1] = spriteIndex;
	Flagpole_scoreSpriteRenderOrder[polePlayerCount - 1] = polePlayerCount - 1; // Store in order received
	Flagpole_scoreSpriteCount = polePlayerCount;

	// Calculate render offsets
	for (u32 i = 0; i < polePlayerCount; i++)
	{
		u32 higherScoreCount = 0;
		for (u32 j = 0; j < polePlayerCount; j++)
		{
			if (Flagpole_scoreSprites[j] < Flagpole_scoreSprites[i])
				higherScoreCount++;
		}
		Flagpole_scoreSpriteRenderOffset[i] = higherScoreCount;
	}
}

void Flagpole_drawSprite(const GXOamAttr* oamAttrs, fx32 x, fx32 y, OAM::Flags flags, u8 palette, u8 affineSet, const Vec2* scale, s16 rot, const s16 rotCenter[2], OAM::Settings settings)
{
	for (u32 i = 0; i < Flagpole_scoreSpriteCount; i++)
	{
		u8 renderIndex = Flagpole_scoreSpriteRenderOrder[i];
		u8 spriteIndex = Flagpole_scoreSprites[renderIndex];

		GXOamAttr** attrs = rcast<GXOamAttr**>(0x0216FD50);
		fx32 offset = Flagpole_scoreSpriteRenderOffset[i] * 0x6000;
		OAM::drawSprite(attrs[spriteIndex - 1], x, y + offset, flags, palette, affineSet, scale, rot, rotCenter, settings);
	}
}

ncp_set_call(0x0212FB48, 12, Flagpole_drawSprite)
ncp_set_call(0x0212FC70, 12, Flagpole_drawSprite)

ncp_asmfunc void Flagpole_getClosestPlayer_ASM()
{asm(R"(
ncp_over(0x0212FCBC, 12)
	BL      _Z27ActorFixes_getClosestPlayerP10StageActor
	NOP
ncp_endover()
)");}

// Player -------------------------------------------------------------------------------

fx32 Player_sendFlyingVelocities[2] = { -0x5800, 0x5800 };

bool Player_missedPoleState(Player* self, void* arg)
{
	s8& step = self->transitionStateStep;

	if (step == Func::Init)
	{
		step = 1;
		self->velocity.x = 0;
		return true;
	}
	if (step == Func::Exit)
	{
		return true;
	}

	if (step == 1)
	{
		self->updateGravityAcceleration();
		self->updateVerticalVelocityClamped();
		self->applyVelocity();

		bool grounded = (scast<u32>(self->collisionMgr.updatePlayerGroundCollision()) & CollisionMgr::Result::GroundAny);
		if (!grounded)
		{
			self->setAnimation(6, true, Player::FrameMode::Restart, 1fx);
		}
		else
		{
			step = 2;
		}
	}
	else if (step == 2)
	{
		self->setAnimation(0, true, Player::FrameMode::Restart, 1fx);
	}

	self->updateAnimation();
	return true;
}

void Player_beginMissedPoleState(Player* self)
{
	self->switchMainState(&Player::idleState);
	self->switchTransitionState(ptmf_cast(Player_missedPoleState));
}

bool Player_sentFlyingWithPoleState(Player* self, void* arg)
{
	s8& step = self->transitionStateStep;

	if (step == Func::Init)
	{
		step = 1;

		PlayerSpectate::setTarget(self->linkedPlayerID, Stage_flagpoleLinkedPlayer->linkedPlayerID);

		self->velocity.x = Player_sendFlyingVelocities[Flagpole_instance->direction];
		self->velocity.y = -0x2000;

		return true;
	}
	if (step == Func::Exit)
	{
		return true;
	}

	self->applyVelocity();

	self->rotation.z =
		Flagpole_instance->direction ?
			(self->rotation.z - 0x800) :
			(self->rotation.z + 0x800);

	self->updateAnimation();
	return true;
}

void Player_beginSentFlyingAwayWithPoleState(Player* self)
{
	self->switchMainState(&Player::idleState);
	self->switchTransitionState(ptmf_cast(Player_sentFlyingWithPoleState));
}

// Give some time for other players to reach the goal

ncp_jump(0x0211B5C8, 10)
bool Player_flagpoleTransitState_OVERRIDE(Player* self, void* arg)
{
	if (self->transitionStateStep == 3 && Flagpole_waitPlayerCountdown != 0)
	{
		if (self == Stage_flagpoleLinkedPlayer)
		{
			Flagpole_waitPlayerCountdown--;

			if (Flagpole_waitPlayerCountdown == 0)
				Flagpole_switchToPlayerSlideState(Flagpole_instance);
		}

		self->updateAnimation();
		return true;
	}

	// Only the linkedPlayer finishes the level
	if (self->transitionStateStep == 16 && self != Stage_flagpoleLinkedPlayer)
		return true;

	return Player_flagpoleTransitState_SUPER(self, arg);
}

ncp_repl(0x0211B67C, 10, "NOP") // Do not freeze other players on goal

ncp_repl(0x0211B688, 10, "NOP") // Do not stop BGM, do it in Flagpole_switchToPlayerSlideState instead

// Use stack-like positioning for the players on the pole

ncp_repl(0x02117FAC, 10, "MOV R0, R4")

ncp_call(0x02117FB8, 10)
u32 Player_customGoalSlideCollisionCheck(Player* self)
{
	fx32 targetY = Flagpole_instance->position.y + Flagpole_getAccumulatedHeightsBelow(self);

	if (self->position.y > targetY)
		return 0;

	self->position.y = targetY;
	return 0x1000;
}

// Wait for all players to grab the pole

ncp_repl(0x0211B90C, 10, "NOP") // Set in Player_customBeginPoleSlide instead

ncp_call(0x0211B91C, 10)
void Player_customBeginPoleSlide(Player* self)
{
	if (Flagpole_allPlayersSlidingPole())
	{
		self->transitionStateStep = 4;
		self->goalBeginPoleSlide();
	}
}

void Player_customBeginPoleJump(Player* self)
{
	// Only the linkedPlayer finishes the level
	if (self == Stage_flagpoleLinkedPlayer)
	{
		self->fireworksToSpawn = self->playGoalFanfare();
        self->transitionTimer = 360;
		*rcast<u32*>(0x020CA8C0) |= 12; // levelEndBitmask
	}

	// Higher players jump further
	fx32 targetH = 0x1800 + 0x800 * Flagpole_playerOrdinal[self->linkedPlayerID];

	self->velH = targetH;
    self->targetVelH = targetH;
}

ncp_asmfunc void Flagpole_customBeginPoleJump_ASM()
{asm(R"(
ncp_over(0x0211B9B8, 10)
	MOV     R0, R4
	BL      _Z26Player_customBeginPoleJumpP6Player
	B       0x0211BDD8
ncp_endover()
)");}

ncp_call(0x0211BB84, 10)
bool Player_customGoalUpdatePowerupState(Player* self)
{
	self->updatePowerupState();
	return self->transitionFlag.megaShrink;
}

// Wait for all players to land

ncp_call(0x0211BA18, 10)
bool Player_customGoalJumpLandCompleted(Player* self)
{
	u32 polePlayerCount;
	Player* polePlayers[FlagpoleMaxPlayerCount];
	Flagpole_getPlayersGrabbing(&polePlayerCount, polePlayers, nullptr, nullptr, nullptr);

	for (u32 i = 0; i < polePlayerCount; i++)
	{
		if (!polePlayers[i]->isBodyAnimationCompleted())
			return false;
	}

	return true;
}
