#pragma once

#include <nsmb/core/system/save.hpp>

#include "Packet.hpp"

namespace DesyncGuard
{
	extern MainSave backupSave;

	void storeState();
	void restoreState();
	//void restoreSync(Packet& packet, u8 senderAid, Net::OnPacketTransferComplete completeFunc, void* completeArg = nullptr);
	void markDesyncCheck();
	void markRngDesyncCheck();
}
