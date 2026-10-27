#include "coop/fixes/flagpole/Flagpole.hpp"

#include <nsmb/game/stage/entity.hpp>
#include <nsmb/game/stage/player.hpp>
#include <nsmb/core/system/function.hpp>
#include <nsmb/core/graphics/fader.hpp>
#include <nsmb/core/graphics/2d/oam.hpp>

#include "coop/Coop.hpp"
#include "coop/CoopStage.hpp"
#include "coop/fixes/flagpole/Player.hpp"

struct Flagpole_PTMF
{
	bool (*func)(StageEntity*);
	u32 adj;
};

namespace CoopFixes::Flagpole {

StageEntity* instance;
// Lowest player has value 0, second has 1, third has 2
u8 playerOrdinals[Coop::MaxPlayerCount];
u8 waitPlayerCountdown;
u8 scoreSprites[Coop::MaxPlayerCount];
u8 scoreSpriteRenderOrders[Coop::MaxPlayerCount];
u8 scoreSpriteRenderOffsets[Coop::MaxPlayerCount];
u8 scoreSpriteCount;

extern "C" {
	void CoopSym_setSecretExitFromSpriteData(u32);
	bool CoopSym_Flagpole_switchState(StageEntity* self, Flagpole_PTMF* function);
	bool CoopSym_Flagpole_updateGoalGrab(StageEntity* self);
	extern Flagpole_PTMF CoopSym_Flagpole_sPlayerSlide;
}

void getPlayersGrabbing(u32* polePlayerCount, ::Player** polePlayers, u32* notPolePlayerCount, ::Player** notPolePlayers, bool* allGrabbing)
{
	bool _allGrabbing = true;
	u32 _polePlayerCount = 0;
	u32 _notPolePlayerCount = 0;

	for (u32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
	{
		if (Game::getPlayerDead(playerID))
			continue;

		::Player* player = Game::getPlayer(playerID);
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

void calculatePlayerOrdinals(u32 playerCount, ::Player** players)
{
	for (u32 i = 0; i < playerCount; i++)
	{
		u32 ordinal = 0;

		for (u32 j = 0; j < playerCount; j++)
		{
			if (i != j && players[j]->position.y < players[i]->position.y)
				ordinal++;
		}

		playerOrdinals[players[i]->linkedPlayerID] = ordinal;
	}
}

void beginLevelEnd()
{
	*rcast<u32*>(0x020CA8C0) |= 3; // levelEndBitmask
	rcast<::Player*>(0)->stopBGM(32);
}

void switchToPlayerSlideState(StageEntity* self)
{
	u32 polePlayerCount;
	::Player* polePlayers[Coop::MaxPlayerCount];
	u32 notPolePlayerCount;
	::Player* notPolePlayers[Coop::MaxPlayerCount];
	getPlayersGrabbing(&polePlayerCount, polePlayers, &notPolePlayerCount, notPolePlayers, nullptr);

	calculatePlayerOrdinals(polePlayerCount, polePlayers);

	CoopSym_Flagpole_switchState(self, &CoopSym_Flagpole_sPlayerSlide);

	for (u32 i = 0; i < notPolePlayerCount; i++)
	{
		::Player* player = notPolePlayers[i];
		Player::beginMissedPoleState(player);
	}

	beginLevelEnd();
}

void sendPlayersFlyingAway()
{
	u32 polePlayerCount;
	::Player* polePlayers[Coop::MaxPlayerCount];
	getPlayersGrabbing(&polePlayerCount, polePlayers, nullptr, nullptr, nullptr);

	for (u32 i = 0; i < polePlayerCount; i++)
	{
		::Player* player = polePlayers[i];
		if (player != CoopStage::flagpoleLinkedPlayer) // Because Mega breaking the pole counts as grabbing
			Player::beginSentFlyingAwayWithPoleState(player);
	}
}

bool allPlayersSlidingPole()
{
	u32 polePlayerCount;
	::Player* polePlayers[Coop::MaxPlayerCount];
	getPlayersGrabbing(&polePlayerCount, polePlayers, nullptr, nullptr, nullptr);

	for (u32 i = 0; i < polePlayerCount; i++)
	{
		if (!polePlayers[i]->actionFlag.flagpoleSlide)
			return false;
	}

	return true;
}

void fixFinishSlide()
{
	u32 polePlayerCount;
	::Player* polePlayers[Coop::MaxPlayerCount];
	getPlayersGrabbing(&polePlayerCount, polePlayers, nullptr, nullptr, nullptr);

	for (u32 i = 0; i < polePlayerCount; i++)
		polePlayers[i]->actionFlag.flagpoleEnd = true;
}

ncp_asmfunc void allPlayersSliding_ASM()
{asm(R"(
ncp_over(0x02130154, 12)
	BL      _ZN9CoopFixes8Flagpole21allPlayersSlidingPoleEv
	CMP     R0, #0
	BEQ     0x021301BC
	NOP
	NOP
	NOP
ncp_endover()

ncp_over(0x021301B0, 12)
	BL      _ZN9CoopFixes8Flagpole14fixFinishSlideEv
	NOP
	NOP
ncp_endover()
)");}

ncp_repl(0x0213056C, 12, "NOP") // Only switch the state when everyone grabbed to keep the pole tangible

fx32 getPlayerHeight(::Player* player, ::Player* playerAbove)
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

fx32 getAccumulatedHeightsBelow(::Player* targetPlayer)
{
	u32 polePlayerCount;
	::Player* polePlayers[Coop::MaxPlayerCount];
	getPlayersGrabbing(&polePlayerCount, polePlayers, nullptr, nullptr, nullptr);

	fx32 accumulatedHeight = 0;
	u8 targetOrdinal = playerOrdinals[targetPlayer->linkedPlayerID];

	// Sum up heights of all players with lower ordinals (below the target player)
	for (u32 i = 0; i < polePlayerCount; i++)
	{
		::Player* player = polePlayers[i];
		u8 playerOrdinal = playerOrdinals[player->linkedPlayerID];

		if (playerOrdinal < targetOrdinal)
		{
			// Find the player directly above this one
			::Player* playerAbove = nullptr;
			u8 nextHigherOrdinal = playerOrdinal + 1;

			for (u32 j = 0; j < polePlayerCount; j++)
			{
				if (playerOrdinals[polePlayers[j]->linkedPlayerID] == nextHigherOrdinal)
				{
					playerAbove = polePlayers[j];
					break;
				}
			}

			accumulatedHeight += getPlayerHeight(player, playerAbove);
		}
	}

	return accumulatedHeight;
}

void adjustPlayerPositions(StageEntity* self, ::Player* grabber)
{
	// TODO: this function is not fully prepared for more than 2 players

	u32 polePlayerCount;
	::Player* polePlayers[Coop::MaxPlayerCount];
	getPlayersGrabbing(&polePlayerCount, polePlayers, nullptr, nullptr, nullptr);

	// Find overlapping players and adjust positions
	for (u32 i = 0; i < polePlayerCount; i++)
	{
		::Player* player = polePlayers[i];
		if (player == grabber)
			continue; // Skip the grabber

		u32 playerHeight = getPlayerHeight(player, grabber);
		u32 halfPlayerHeight = playerHeight >> 1;
		u32 grabberHeight = getPlayerHeight(grabber, nullptr);
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

void afterTouched(StageEntity* self)
{
	u16& grabberID = rcast<u16*>(self)[0x756 / 2];

	::Player* grabber = Game::getPlayer(grabberID);

	if (grabber->currentPowerup == PowerupState::Mega)
	{
		CoopStage::flagpoleLinkedPlayer = grabber; // Mega player now owns the flagpole
		instance = self;

		*rcast<u8*>(0x020CA898) |= 0x40; // Stop time counter
		beginLevelEnd();

		sendPlayersFlyingAway();
		return;
	}

	if (CoopStage::flagpoleLinkedPlayer == nullptr) // Pole grabbed for the first time
	{
		CoopStage::flagpoleLinkedPlayer = grabber;
		waitPlayerCountdown = 180; // 3 seconds (same as NSMB Wii)
		instance = self;

		*rcast<u8*>(0x020CA898) |= 0x40; // Stop time counter
	}

	// Adjust player positions to prevent overlap whenever someone grabs the pole
	adjustPlayerPositions(self, grabber);

	// Check if everyone is grabbing the pole

	bool allGrabbing;
	getPlayersGrabbing(nullptr, nullptr, nullptr, nullptr, &allGrabbing);

	if (!allGrabbing)
		return;

	// Everyone is grabbing the pole, proceed with the pole slide

	waitPlayerCountdown = 0;
	switchToPlayerSlideState(self);
}

ncp_asmfunc void afterTouched_ASM()
{asm(R"(
ncp_over(0x02130570, 12)
	MOV     R0, R6
	BL      _ZN9CoopFixes8Flagpole12afterTouchedEP11StageEntity
	B       0x021305B8
ncp_endover()
)");}

ncp_repl(0x02130588, 12, "NOP") // Do it in Flagpole_beginLevelEnd

ncp_repl(0x02130698, 12, "MOV R0, R6") // Pass Flagpole* instead of Flagpole*->settings

ncp_call(0x0213069C, 12)
void storeScoreSprite(StageEntity* self)
{
	CoopSym_setSecretExitFromSpriteData(self->settings); // Keep replaced instruction

	u32 polePlayerCount;
	getPlayersGrabbing(&polePlayerCount, nullptr, nullptr, nullptr, nullptr);

	u8 spriteIndex = rcast<u16*>(self)[0x754 / 2];
	scoreSprites[polePlayerCount - 1] = spriteIndex;
	scoreSpriteRenderOrders[polePlayerCount - 1] = polePlayerCount - 1; // Store in order received
	scoreSpriteCount = polePlayerCount;

	// Calculate render offsets
	for (u32 i = 0; i < polePlayerCount; i++)
	{
		u32 higherScoreCount = 0;
		for (u32 j = 0; j < polePlayerCount; j++)
		{
			if (scoreSprites[j] < scoreSprites[i])
				higherScoreCount++;
		}
		scoreSpriteRenderOffsets[i] = higherScoreCount;
	}
}

void drawSprite(const GXOamAttr* oamAttrs, fx32 x, fx32 y, OAM::Flags flags, u8 palette, u8 affineSet, const Vec2* scale, s16 rot, const s16 rotCenter[2], OAM::Settings settings)
{
	for (u32 i = 0; i < scoreSpriteCount; i++)
	{
		u8 renderIndex = scoreSpriteRenderOrders[i];
		u8 spriteIndex = scoreSprites[renderIndex];

		GXOamAttr** attrs = rcast<GXOamAttr**>(0x0216FD50);
		fx32 offset = scoreSpriteRenderOffsets[i] * 0x6000;
		OAM::drawSprite(attrs[spriteIndex - 1], x, y + offset, flags, palette, affineSet, scale, rot, rotCenter, settings);
	}
}

ncp_set_call(0x0212FB48, 12, drawSprite)
ncp_set_call(0x0212FC70, 12, drawSprite)

ncp_asmfunc void getClosestPlayer_ASM()
{asm(R"(
ncp_over(0x0212FCBC, 12)
	BL      _ZN9CoopActor16getClosestPlayerEP10StageActor
	NOP
ncp_endover()
)");}

} // namespace CoopFixes::Flagpole
