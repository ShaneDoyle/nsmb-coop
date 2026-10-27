#pragma once

#include <nsmb/core/entity/scene.hpp>
#include <nsmb/core/system/function.hpp>

#include "objectids/dsimodewarn.hpp"

class DSiModeScene : public Scene
{
public:
	static ObjectProfile Profile;

	constexpr static u32 ObjectID = ObjectID::DSiModeWarn::DSiModeScene;

	s32 onCreate() override;
	s32 onUpdate() override;
	s32 onDestroy() override;
};
