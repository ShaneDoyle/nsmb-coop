#include "dsimodewarn/DSiModeScene.hpp"

#define BASE_SHARED			0x027FF000
#define SRAMCHECKER7		BASE_SHARED + 0xA7E

static inline bool isDSiDevice()
{
	// Read from variable set in ARM7
	return *rcast<vu16*>(SRAMCHECKER7) & 0x8000;
}

static inline bool isDSCpuMode()
{
	// Return true if the instruction was not patched
	return *rcast<vu32*>(0x01FFA780) == 0xE2500004;
}

ncp_call(0x020CCAD0, 1)
ncp_call(0x020CC720, 1)
ncp_thumb void BootScene_goToTitlescreenHook(u32 sceneID, u32 settings)
{
	if (isDSiDevice() && isDSCpuMode())
	{
		Scene::switchScene<DSiModeScene>();
		return;
	}
	Scene::switchScene(sceneID, settings);
}
