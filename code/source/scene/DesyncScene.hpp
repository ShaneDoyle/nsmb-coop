#pragma once

#include <nsmb/core/entity/scene.hpp>
#include <nsmb/core/system/function.hpp>
#include <nsmb/core/net.hpp>

#include "Packet.hpp"

class DesyncScene : public Scene
{
public:
	s32 onCreate() override;
	s32 onUpdate() override;
	s32 onDestroy() override;

	void (*updateFunc)(DesyncScene*);
	s8 updateStep;
	// Packet packet;
	char textBuffer[128];

	static ObjectProfile profile;

	static const char* levelNames[];

	void switchState(void (*updateFunc)(DesyncScene*));

	inline void switchState(void (DesyncScene::*updateFunc)()) {
		switchState(ptmf_cast(updateFunc));
	}

	void mainState();
	void syncState();

	static void initTopGfx();
	static void initSubGfx();
	// static void synchingComplete(u16 aid, void* arg);
};
