#include "coop/fixes/flagpole/Player.hpp"

#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/player.hpp>
#include <nsmb/core/system/function.hpp>

#include "coop/CoopStage.hpp"
#include "coop/CoopCamera.hpp"
#include "coop/fixes/flagpole/Flagpole.hpp"

namespace CoopFixes::Player {

fx32 sendFlyingVelocities[2] = { -0x5800, 0x5800 };

bool missedPoleState(::Player* self, void* arg)
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
			self->setAnimation(6, true, ::Player::FrameMode::Restart, 1fx);
		}
		else
		{
			step = 2;
		}
	}
	else if (step == 2)
	{
		self->setAnimation(0, true, ::Player::FrameMode::Restart, 1fx);
	}

	self->updateAnimation();
	return true;
}

void beginMissedPoleState(::Player* self)
{
	self->switchMainState(&::Player::idleState);
	self->switchTransitionState(ptmf_cast(missedPoleState));
}

bool sentFlyingWithPoleState(::Player* self, void* arg)
{
	s8& step = self->transitionStateStep;

	if (step == Func::Init)
	{
		step = 1;

		CoopCamera::setFollowTarget(self->linkedPlayerID, CoopStage::flagpoleLinkedPlayer->linkedPlayerID);

		self->velocity.x = sendFlyingVelocities[Flagpole::instance->direction];
		self->velocity.y = -0x2000;

		return true;
	}
	if (step == Func::Exit)
	{
		return true;
	}

	self->applyVelocity();

	self->rotation.z =
		Flagpole::instance->direction ?
			(self->rotation.z - 0x800) :
			(self->rotation.z + 0x800);

	self->updateAnimation();
	return true;
}

void beginSentFlyingAwayWithPoleState(::Player* self)
{
	self->switchMainState(&::Player::idleState);
	self->switchTransitionState(ptmf_cast(sentFlyingWithPoleState));
}

// Give some time for other players to reach the goal

ncp_asmfunc bool flagpoleTransitState_SUPER(::Player* self, void* arg)
{asm(R"(
	PUSH    {R4,LR}
	B       0x0211B5CC
)");}

ncp_jump(0x0211B5C8, 10)
bool flagpoleTransitState_OVERRIDE(::Player* self, void* arg)
{
	if (self->transitionStateStep == 3 && Flagpole::waitPlayerCountdown != 0)
	{
		if (self == CoopStage::flagpoleLinkedPlayer)
		{
			Flagpole::waitPlayerCountdown--;

			if (Flagpole::waitPlayerCountdown == 0)
				Flagpole::switchToPlayerSlideState(Flagpole::instance);
		}

		self->updateAnimation();
		return true;
	}

	// Only the linkedPlayer finishes the level
	if (self->transitionStateStep == 16 && self != CoopStage::flagpoleLinkedPlayer)
		return true;

	return flagpoleTransitState_SUPER(self, arg);
}

ncp_repl(0x0211B67C, 10, "NOP") // Do not freeze other players on goal

ncp_repl(0x0211B688, 10, "NOP") // Do not stop BGM, do it in Flagpole::switchToPlayerSlideState instead

// Use stack-like positioning for the players on the pole

ncp_repl(0x02117FAC, 10, "MOV R0, R4")

ncp_call(0x02117FB8, 10)
u32 customGoalSlideCollisionCheck(::Player* self)
{
	fx32 targetY = Flagpole::instance->position.y + Flagpole::getAccumulatedHeightsBelow(self);

	if (self->position.y > targetY)
		return 0;

	self->position.y = targetY;
	return 0x1000;
}

// Wait for all players to grab the pole

ncp_repl(0x0211B90C, 10, "NOP") // Set in customBeginPoleSlide instead

ncp_call(0x0211B91C, 10)
void customBeginPoleSlide(::Player* self)
{
	if (Flagpole::allPlayersSlidingPole())
	{
		self->transitionStateStep = 4;
		self->goalBeginPoleSlide();
	}
}

void customBeginPoleJump(::Player* self)
{
	// Only the linkedPlayer finishes the level
	if (self == CoopStage::flagpoleLinkedPlayer)
	{
		self->fireworksToSpawn = self->playGoalFanfare();
        self->transitionTimer = 360;
		*rcast<u32*>(0x020CA8C0) |= 12; // levelEndBitmask
	}

	// Higher players jump further
	fx32 targetH = 0x1800 + 0x800 * Flagpole::playerOrdinals[self->linkedPlayerID];

	self->velH = targetH;
    self->targetVelH = targetH;
}

ncp_asmfunc void customBeginPoleJump_ASM()
{asm(R"(
ncp_over(0x0211B9B8, 10)
	MOV     R0, R4
	BL      _ZN9CoopFixes6Player19customBeginPoleJumpEP6Player
	B       0x0211BDD8
ncp_endover()
)");}

// Wait for all players to land

ncp_call(0x0211BA18, 10)
bool customGoalJumpLandCompleted(::Player* self)
{
	u32 polePlayerCount;
	::Player* polePlayers[Coop::MaxPlayerCount];
	Flagpole::getPlayersGrabbing(&polePlayerCount, polePlayers, nullptr, nullptr, nullptr);

	for (u32 i = 0; i < polePlayerCount; i++)
	{
		if (!polePlayers[i]->isBodyAnimationCompleted())
			return false;
	}

	return true;
}

} // namespace CoopFixes::Player
