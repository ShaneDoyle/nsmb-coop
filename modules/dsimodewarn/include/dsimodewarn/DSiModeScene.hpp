#pragma once

#include <nsmb/core/entity/scene.hpp>
#include <nsmb/core/system/function.hpp>

class DSiModeScene : public Scene
{
public:
	static ObjectProfile profile;

	s32 onCreate() override;
	s32 onUpdate() override;
	s32 onDestroy() override;
};
