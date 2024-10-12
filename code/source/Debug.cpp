#ifdef NTR_DEBUG

#include "nsmb_nitro.hpp"

#include "nsmb/entity/scene.hpp"
#include "nsmb/system/input.hpp"
#include "nsmb/system/function.hpp"
#include "util/collisionviewer.hpp"
#include "util/playerdragger.hpp"

#include "PlayerSpectate.hpp"

namespace Debug
{
	bool collisionViewerEnabled = false;
	bool draggingPlayer[2];

	void update()
	{
		for (s32 i = 0; i < Game::getPlayerCount(); i++)
		{
			u16 keysHeld = Input::getHeldKeys(i);
			u16 keysPressed = Input::getPressedKeys(i);
			if ((keysHeld & Keys::Select) && (keysPressed & Keys::X))
			{
				collisionViewerEnabled = !collisionViewerEnabled;
			}

			if (keysPressed & Keys::B)
			{
				PlayerSpectate::setTarget(i, !PlayerSpectate::getTarget());
			}

			if (Scene::currentSceneID == u16(SceneID::Stage))
			{
				if (keysHeld & Keys::X)
				{
					if (!draggingPlayer[i])
					{
						PlayerDragger::beginDrag(Game::getPlayer(i));
						draggingPlayer[i] = true;
					}
				}
				else
				{
					if (draggingPlayer[i])
					{
						PlayerDragger::endDrag(Game::getPlayer(i));
						draggingPlayer[i] = false;
					}
				}
			}
		}
	}

	void render3D()
	{
		if (collisionViewerEnabled && Scene::currentSceneID == u16(SceneID::Stage))
		{
			CollisionViewer::render();
		}
	}
}

ncp_jump(0x0200A6C0)
NTR_NAKED void inputFetchedHook() {asm(R"(
	BL      _ZN5Debug6updateEv
	ADD     SP, #4
	POP     {R4-R11,PC}
)");}

ncp_call(0x02004EFC)
NTR_NAKED void preSwapBufferHook() {asm(R"(
	BL      _ZN5Debug8render3DEv
	BL      NNS_G3dGeFlushBuffer
	B       0x02004EFC + 4
)");}

#endif
