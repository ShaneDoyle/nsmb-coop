#include "InfoTextBoxBlock.hpp"

#include <nsmb/game/game.hpp>
#include <nsmb/game/stage/player.hpp>
#include <nsmb/core/math/matrix.hpp>
#include <nsmb/core/system/input.hpp>
#include <nsmb/core/filesystem/cache.hpp>

constexpr u32 bmgFileID = 2091 - 131;
constexpr u32 bmdFileID = 2092 - 131;

static TextBox& textBox = *rcast<TextBox*>(0x020CA9B0);
static u8& isTimeStopped = *rcast<u8*>(0x020CA898);
static u32& someFreezeBitmask = *rcast<u32*>(0x020CA8D0);

ActorProfile InfoTextBoxBlock::profile = {
	&constructObject<InfoTextBoxBlock>,
	0xFB, 0x69,
	InfoTextBoxBlock::loadResources
};

ColliderInfo InfoTextBoxBlock::colliderInfo =
{
	-0x8000, //left
	0x8000, //top
	0x8000, //right
	-0x8000, //bottom
	{
		InfoTextBoxBlock::hitFromTop, //callbackTop
		InfoTextBoxBlock::hitFromBottom, //callbackBottom
		nullptr, //callbackSide
	}
};

s32 InfoTextBoxBlock::onCreate()
{
	if (!prepareResourcesSafe(64, Memory::gameHeapPtr))
		return 0;

	rotationTranslation = Vec2(0);
	renderOffset = Vec2(0);
	fogFlag = false;
	alpha = -1;

	dialogVisible = false;

	collider.init(this, colliderInfo, 0, 0, &scale);
	collider.link();

	return 1;
}

s32 InfoTextBoxBlock::onUpdate()
{
	destroyInactive(0);
	return 1;
}

s32 InfoTextBoxBlock::onRender()
{
	StageEntity3D::onRender();

	if (dialogVisible)
	{
		textBox.renderAButton(0, 0);
		textBox.renderText(0, 0);
		TextBox::renderOpaqueBox(0, 0, 0);

		if (Input::playerKeysPressed[triggerPlayerID] & Keys::A)
		{
			isTimeStopped &= ~0x40;
			someFreezeBitmask |= 4;

			for (u32 playerID = 0; playerID < Game::getPlayerCount(); playerID++)
			{
				Player* player = Game::getPlayer(playerID);
				player->updateLocked = false;
			}

			dialogVisible = false;
		}
	}

	return 1;
}

bool InfoTextBoxBlock::onPrepareResources()
{
	void* bmg = FS::Cache::getFile(bmgFileID);
	if (!bmg)
		return false;

	this->bmg = bmg;

	void* bmd = FS::Cache::getFile(bmdFileID);
	if (!bmd)
		return false;

	this->model.create(bmd, 0, 0);

	return true;
}

void InfoTextBoxBlock::showDialog(u32 playerID)
{
	if (!dialogVisible)
	{
		triggerPlayerID = playerID;

		u32 stringIndex = 0;
		textBox.loadText(bmg, &stringIndex);

		isTimeStopped |= 0x40;
		someFreezeBitmask |= 1;

		for (u32 i = 0; i < Game::getPlayerCount(); i++)
		{
			Player* player = Game::getPlayer(i);
			player->updateLocked = true;
		}

		dialogVisible = true;
	}
}

bool InfoTextBoxBlock::loadResources()
{
	FS::Cache::loadFile(bmgFileID, false);
	FS::Cache::loadFile(bmdFileID, false);
	return true;
}

void InfoTextBoxBlock::hitFromTop(StageActor& _self, StageActor& _other)
{
	InfoTextBoxBlock* self = &scast<InfoTextBoxBlock&>(_self);

	if (_other.actorType != ActorType::Player)
		return;

	Player* other = &scast<Player&>(_other);

	if (!other->actionFlag.groundpounding)
		return;

	self->showDialog(other->linkedPlayerID);
}

void InfoTextBoxBlock::hitFromBottom(StageActor& _self, StageActor& _other)
{
	InfoTextBoxBlock* self = &scast<InfoTextBoxBlock&>(_self);

	if (_other.actorType != ActorType::Player)
		return;

	Player* other = &scast<Player&>(_other);

	if (other->velocity.y <= 0)
		return;

	self->showDialog(other->linkedPlayerID);
}
