#include "Save.hpp"

#include <nsmb/game/stage/player/common.hpp>

namespace SaveExt
{
	bool reloadingSave = false;

	void transferMainSave(Packet& packet, u8 senderAid, Net::OnPacketTransferComplete completeFunc, void* completeArg)
	{
		packet.transfer(senderAid, &Save::mainSave, &Save::mainSave, sizeof(MainSave), completeFunc, completeArg);
	}

	void reloadMainSave()
	{
		reloadingSave = true;
		Save::loadMainSave(Save::optionSave.activeSlot, 0, &Save::mainSave);
		reloadingSave = false;
	}

	Save::ReturnCode readMainSavePatch(u32 slot, MainSave* save)
	{
		if (reloadingSave)
			return Save::ReturnCode::Success;

		return Save::readMainSave(slot, save);
	}
}

ncp_set_call(0x02012E1C, SaveExt::readMainSavePatch)

// Allow Luigi's lives to be saved and loaded
ncp_call(0x02012DB0) u32 call_02012DB0() { return (Game::getPlayerLives(1) << 16) | Game::getPlayerLives(0); }
ncp_call(0x02012E84) void call_02012E84(u32, u32 data)
{
	Game::setPlayerLives(0, data & 0xFFFF);
	Game::setPlayerLives(1, data >> 16);
}

// Allow Luigi's powerup state to be saved
ncp_call(0x02012DE0) u32 call_02012DE0() { return (u32(Game::getPlayerPowerup(1)) << 16) | Game::getPlayerPowerup(0); }
ncp_call(0x02012EBC) void call_02012EBC(u32, u32 data)
{
	Game::setPlayerPowerup(0, data & 0xFFFF);
	Game::setPlayerPowerup(1, data >> 16);
}
ncp_repl(0x02012EB4, "B 0x02012EBC")

// Allow Luigi's inventory powerup to be loaded and saved
ncp_call(0x02012DF0) u8 call_02012DF0() { return (Game::getPlayerInventoryPowerup(1) << 4) | Game::getPlayerInventoryPowerup(0); }
ncp_call(0x02012ECC) void call_02012ECC(u32, u8 data)
{
	Game::setPlayerInventoryPowerup(0, data & 0xF);
	Game::setPlayerInventoryPowerup(1, data >> 4);
}

// Set lives for Luigi too when loading new save
asm(R"(
ncp_jump(0x02012E6C)
	ORRNE   R4, R4, R4, LSL#16
	STRNE   R4, [R0, #0xC]
	B       0x02012E70
)");

asm(R"(
ncp_jump(0x02012FB4)
	LDR     R0, =0x50005
	B       0x02012FB8
)");
