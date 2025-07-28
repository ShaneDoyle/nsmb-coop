#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/player.hpp>
#include <nsmb/core/system/function.hpp>
#include <nsmb/core/graphics/fader.hpp>

#include "Stage.hpp"
#include "PlayerSpectate.hpp"

void Player_beginMissedPoleState(Player* self);
void Player_beginSentFlyingAwayWithPoleState(Player* self);

// Flagpole -----------------------------------------------------------------------------

struct Flagpole_PTMF
{
	bool (*func)(StageEntity*);
	u32 adj;
};

// Lowest player has value 0, second has 1, third has 2
u8 Flagpole_playerOrdinal[2];
u8 Flagpole_waitPlayerCountdown;
StageEntity* Flagpole_instance;

asm(R"(
Flagpole_switchState = 0x02130734
Flagpole_updateGoalGrab = 0x0213042C
Flagpole_sPlayerSlide = 0x02132500

.type Player_flagpoleTransitState_SUPER, %function
Player_flagpoleTransitState_SUPER:
	PUSH    {R4,LR}
	B       0x0211B5CC
)");
extern "C" {
	bool Flagpole_switchState(StageEntity* self, Flagpole_PTMF* function);
	bool Flagpole_updateGoalGrab(StageEntity* self);
	bool Player_flagpoleTransitState_SUPER(Player* self, void* arg);
	Flagpole_PTMF Flagpole_sPlayerSlide;
}

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
	Player* polePlayers[2];
	u32 notPolePlayerCount;
	Player* notPolePlayers[2];
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
	Player* polePlayers[2];
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
	Player* polePlayers[2];
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
	Player* polePlayers[2];
	Flagpole_getPlayersGrabbing(&polePlayerCount, polePlayers, nullptr, nullptr, nullptr);

	for (u32 i = 0; i < polePlayerCount; i++)
		polePlayers[i]->actionFlag.flagpoleEnd = true;
}

asm(R"(
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
)");

ncp_repl(0x0213056C, 12, "NOP") // Only switch the state when everyone grabbed to keep the pole tangible

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

	// Check if everyone is grabbing the pole

	bool allGrabbing;
	Flagpole_getPlayersGrabbing(nullptr, nullptr, nullptr, nullptr, &allGrabbing);

	if (!allGrabbing)
		return;

	// Everyone is grabbing the pole, proceed with the pole slide

	Flagpole_waitPlayerCountdown = 0;
	Flagpole_switchToPlayerSlideState(self);
}

asm(R"(
ncp_over(0x02130570, 12)
	MOV     R0, R6
	BL      _Z21Flagpole_afterTouchedP11StageEntity
	B       0x021305B8
ncp_endover()
)");

ncp_repl(0x02130588, 12, "NOP") // Do it in Flagpole_beginLevelEnd

asm(R"(
ncp_over(0x0212FCBC, 12)
	BL      _Z27ActorFixes_getClosestPlayerP10StageActor
	NOP
ncp_endover()
)");

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
	fx32 targetY = Flagpole_instance->position.y + 0x10000 * Flagpole_playerOrdinal[self->linkedPlayerID];

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
	fx32 targetH = 0x1800 + 0x400 * Flagpole_playerOrdinal[self->linkedPlayerID];

	self->velH = targetH;
    self->targetVelH = targetH;
}

asm(R"(
ncp_over(0x0211B9B8, 10)
	MOV     R0, R4
	BL      _Z26Player_customBeginPoleJumpP6Player
	B       0x0211BDD8
ncp_endover()
)");

ncp_call(0x0211BB84, 10)
bool Player_customGoalUpdatePowerupState(Player* self)
{
	self->updatePowerupState();
	return self->transitionFlag.megaShrink;
}
