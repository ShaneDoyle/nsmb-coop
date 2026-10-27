#pragma once

#include <nsmb/core/system/save.hpp>

#include "coop/Packet.hpp"

namespace CoopSave
{
	void transferMainSave(Packet& packet, u8 senderAid, Net::OnPacketTransferComplete completeFunc, void* completeArg = nullptr);
	void reloadMainSave();
	Save::ReturnCode readMainSavePatch(u32 slot, MainSave* save);
	void restoreOptions();
	void loadBackupParams();
	void saveBackupParams();
}
