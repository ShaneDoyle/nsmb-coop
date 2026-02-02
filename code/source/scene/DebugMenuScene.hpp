#pragma once

#include <nsmb/core/entity/scene.hpp>
#include <nsmb/core/system/function.hpp>
#include <nsmb/core/net.hpp>

#include "Packet.hpp"

class DebugMenuScene : public Scene
{
public:
	s32 onCreate() override;
	s32 onUpdate() override;
	s32 onDestroy() override;

	void (*updateFunc)(DebugMenuScene*);
	s8 updateStep;
	// Packet packet;
	char textBuffer[128];

	static ObjectProfile profile;

	static const char* levelNames[];

	static void clearScreen();

	void switchState(void (*updateFunc)(DebugMenuScene*));

	inline void switchState(void (DebugMenuScene::*updateFunc)()) {
		switchState(ptmf_cast(updateFunc));
	}

	void mainState();

	static void initTopGfx();
	static void initSubGfx();
	// static void synchingComplete(u16 aid, void* arg);
};
