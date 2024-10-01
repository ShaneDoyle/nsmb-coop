#include "nitro_if.h"

#include "nsmb/entity/scene.h"
#include "nsmb/system/input.h"
#include "nsmb/system/function.h"
#include "util/collisionviewer.hpp"
#include "util/playerdragger.hpp"
#include "util/eprintf.h"

namespace Debug
{
	bool collisionViewerEnabled = false;
	bool draggingPlayer[2];

	int tick = 0;

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

		/*if (tick == 60) {
			Base* base = ProcessManager::getNextObjectByObjectID(Scene::currentSceneID, 0);
			SceneNode* node = base->link.linkConnect.getNext();
			while (node != nullptr) {
				eprintf("%d ", node->object->id);
				node = node->getNext();
			}
			eprintf("\n");

			tick = 0;
		}
		tick++;*/
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
