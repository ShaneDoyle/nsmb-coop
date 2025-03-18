#pragma once

#include <nsmb/core/system/save.hpp>

#include "Packet.hpp"

namespace SaveExt
{
	void transferMainSave(Packet& packet, u8 senderAid, Net::OnPacketTransferComplete completeFunc, void* completeArg = nullptr);
	void reloadMainSave();
	Save::ReturnCode readMainSavePatch(u32 slot, MainSave* save);
}
