#include "DesyncGuard.hpp"

#include <nsmb/game/stage/player.hpp>
#include <nsmb/core/net.hpp>
#include <nsmb/core/wifi.hpp>
#include <nsmb/core/system/save.hpp>
#include <nsmb/core/entity/scene.hpp>
#include <nsmb/core/graphics/fader.hpp>

#include "Save.hpp"

// Notes:
// Some desync check marks are set in other sources
// RNG uses another marker to avoid making the other checks useless if RNG is called every update

asm(R"(
Stage_switchScene = 0x020A183C

PlayerBase_requestPowerupSwitch_SUPER:
	LDRB    R2, [R0,#0x7A9]
	B       0x0212B9B0

Net_getRandom_SUPER:
	PUSH    {LR}
	B       0x0200E6F8
)");
extern "C" {
	void Stage_switchScene(u32 sceneID, u32 settings);
	bool PlayerBase_requestPowerupSwitch_SUPER(PlayerBase* self, PowerupState powerup);
	u32 Net_getRandom_SUPER();
}

namespace DesyncGuard
{
	constexpr u32 markerIndex = 2;
	constexpr u32 rngMarkerIndex = 3;

	MainSave backupSave;

	void storeState()
	{
		MI_CpuCopyFast(&Save::mainSave, &backupSave, sizeof(MainSave));
	}

	void restoreState()
	{
		Net::syncRandomFast();

		MI_CpuCopyFast(&backupSave, &Save::mainSave, sizeof(MainSave));

		SaveExt::reloadMainSave();
	}

	/*
	void restoreSync(Packet& packet, u8 senderAid, Net::OnPacketTransferComplete completeFunc, void* completeArg)
	{
		Net::syncRandomFast();

		if (senderAid == Net::localAid)
		{
			// If we are the sender, restore the save before sending it
			MI_CpuCopyFast(&backupSave, &Save::mainSave, sizeof(MainSave));
		}

		SaveExt::transferMainSave(packet, senderAid, completeFunc, completeArg);
	}
	*/

	void switchToDesyncScene()
	{
		if (Scene::currentSceneID == scast<u16>(SceneID::Stage))
		{
			Stage_switchScene(255, 0);
			return;
		}
		Scene::switchScene(255, 0);
	}

	// Marks a desync checkpoint
	void markDesyncCheck()
	{
		if (!Net::isConnected())
			return;

		Net::Core::setMarker(markerIndex);
	}

	// Marks an RNG desync checkpoint
	void markRngDesyncCheck()
	{
		if (!Net::isConnected())
			return;

		Net::Core::setMarker(rngMarkerIndex);
	}

	// Checks if the marker has different values between consoles
	bool checkMarkerDesync(u32 curMarkerIndex)
	{
		u32 markerMask = 1 << curMarkerIndex;

		u32 initialValue = -1;

		for (u32 aid = 0; aid < 4; aid++)
		{
			bool notConnected = (rcast<u8*>(Net::consoleStates)[aid] & 2) == 0;
			if (notConnected)
				continue;

			u32 value = (markerMask & Net::getPacketByte(aid, 41)) != 0;

			if (initialValue == -1)
			{
				initialValue = value;
			}
			else if (value != initialValue)
			{
				return true;
			}
		}

		return false;
	}

	void update()
	{
		if (!Net::isConnected() || Net::packetTransIntegrity != Net::PacketTransferIntegrity::PacketOrderIntegrity)
			return;

		if (checkMarkerDesync(markerIndex) || checkMarkerDesync(rngMarkerIndex))
		{
			// Switching to the desync scene will set the marker again
			// but that's fine because it gets cleared right away
			switchToDesyncScene();
		}

		// Marker should only ever live for 1 update
		Net::clearMarker(markerIndex);
		Net::clearMarker(rngMarkerIndex);
	}

	void MainGame_loop_CALL()
	{
		Net::update(); // Keep replaced instruction

		update();
	}

	void Scene_switchScene_CALL(Fader& fader)
	{
		fader.prepareFadeOut(); // Keep replaced instruction

		markDesyncCheck();
	}

	void Player_damage_CALL(Player* player)
	{
		player->tryReleaseHeldActor(); // Keep replaced instruction

		markDesyncCheck();
	}

	bool PlayerBase_requestPowerupSwitch_OVERRIDE(PlayerBase* player, PowerupState powerup)
	{
		bool ret = PlayerBase_requestPowerupSwitch_SUPER(player, powerup); // Keep replaced instruction

		markDesyncCheck();

		return ret;
	}

	u32 Net_getRandom_OVERRIDE()
	{
		markRngDesyncCheck();

		return Net_getRandom_SUPER(); // Keep replaced instruction
	}
}

ncp_set_call(0x02004F18, DesyncGuard::MainGame_loop_CALL)
ncp_set_call(0x0201323C, DesyncGuard::Scene_switchScene_CALL)
ncp_set_call(0x02104AB8, 10, DesyncGuard::Player_damage_CALL)
ncp_set_jump(0x0212B9AC, 11, DesyncGuard::PlayerBase_requestPowerupSwitch_OVERRIDE)
ncp_set_jump(0x0200E6F4, DesyncGuard::Net_getRandom_OVERRIDE)
