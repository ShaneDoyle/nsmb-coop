#include "coop/CoopActor.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/game/stage/player/player.hpp>
#include <nsmb/game/stage/entity3danm.hpp>
#include <nsmb/core/system/function.hpp>
#include <nsmb/core/math/math.hpp>

// Notes:
//  - Any actor using `Model::getNodePosition` (0x020196DC) or `Model::getNodeMatrix` (0x0201972C) that does
//    not override `skipRender` to always render but become invisible must use `CoopActor::safeSkipRender`.
//  - Any actor using `Game::isOutsideCamera(..., Game::localPlayerID)` (0x0200AE9C)
//    to make decisions beyond just rendering must use `CoopActor::isOutsideCamera`.

namespace CoopActor {

u8 matchPlayerID = NO_MATCH; // Must be u32, used in LDR offset replacements
u8 matchZoneID = NO_MATCH;

// Replacement for StageEntity::skipRender that updates the model but doesn't draw it
bool safeSkipRender(StageEntity3DAnm* self)
{
	self->StageEntity::skipRender() ?
		self->model.disableRendering() :
		self->model.enableRendering();
	return false;
}

// Replacement for Game::getLocalPlayer in some cases
Player* getClosestPlayer(StageActor* self)
{
	return self->getClosestPlayer(nullptr, nullptr);
}

// Replacement for Game::isOutsideCamera(..., Game::localPlayerID)
bool isOutsideCamera(StageActor* self, const FxRect& boundingBox/*, u8 playerID*/)
{
	Player* player = getClosestPlayer(self);
	return Stage::isOutsideCamera(self->position, boundingBox, player->linkedPlayerID);
}

Player* getClosestPlayerInZone(StageActor* self, u32 zoneID)
{
	matchZoneID = zoneID;
	Player* player = getClosestPlayer(self);
	matchZoneID = NO_MATCH;
	return player;
}

u8 getHorizontalDirectionToPlayer(StageEntity* self, const Vec3& position, u32 playerID)
{
	matchPlayerID = playerID;
	u8 direction = self->getHorizontalDirectionToPlayer(position);
	matchPlayerID = NO_MATCH;
	return direction;
}

bool isPlayerInRange(StageActor* self, Player* player)
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
ncp_thumb bool isInRangeOfAllPlayers(StageEntity* self)
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

	return isOutsideCamera(self, boundingBox);
}

Player* getClosestPlayerFilter(s32 playerID)
{
	if ((matchPlayerID != NO_MATCH && matchPlayerID != playerID) || Game::getPlayerDead(playerID))
		return nullptr;

	Player* player = Game::getPlayer(playerID);

	if (player == nullptr || (matchZoneID != NO_MATCH && !isPlayerInZone(player, matchZoneID)))
		return nullptr;

	return player;
}

ncp_set_call(0x020A0544, 0, getClosestPlayerFilter)
ncp_set_call(0x020A0628, 0, getClosestPlayerFilter)

ncp_asmfunc Player* StageActor_getClosestPlayer_SUPER(StageActor* self, s32* distanceX, s32* distanceY)
{asm(R"(
	PUSH    {R4,LR}
	B       0x020A06A0
)");}

ncp_jump(0x020A069C, 0)
Player* StageActor_getClosestPlayer_OVERRIDE(StageActor* self, s32* distanceX, s32* distanceY)
{
	Player* player = StageActor_getClosestPlayer_SUPER(self, distanceX, distanceY);

	// Make sure a null player can only be returned
	// when using CoopActor::getClosestPlayerInZone
	if (player == nullptr && matchZoneID == NO_MATCH)
		return Game::getPlayer(0);

	return player;
}

} // namespace CoopActor
