#include <nsmb/core/entity/scene.hpp>
#include <nsmb/game/vsconnect/scene.hpp>
// The fields of the VSConnectScene class are not yet documented, raw access to fields is required.
#include <nsmb/game/sound/sound.hpp>
#include <nsmb/core/net.hpp>

#include "Widescreen.hpp"

ncp_call(0x021592DC, 52)
u32 VSConnect_skipFirstSubMenu()
{
	register VSConnectScene* self asm("r4");

	rcast<u32*>(self)[0x15C / 4] = rcast<u32>(&VSConnectScene::charSelectSM); // Sub-menu updater
	rcast<u32*>(self)[0x160 / 4] = 1; // Sub-menu swap timer

	return 1;
}

ncp_call(0x02158800, 52)
void VSConnect_modifyReturn(VSConnectScene* self, VSConnectScene::SubMenu* subMenu, s32 delay, bool playSound)
{
	self->scheduleSubMenuChange(subMenu, delay, playSound); // Call replaced function

	// Variable that happens to not be 0 during connection menu
	if (rcast<u32*>(self)[0x134 / 4])
	{
		Scene::switchScene(SceneID::TitleScreen, 0);
		SND::stopBGM(30);
	}
}

void VSConnect_onPacketBufferReceived(u16 aid, void* arg)
{
	u32 wifiMode = rcast<u32*>(arg)[0x168/4];
	Net::PacketBuffer* pktBuf = rcast<Net::PacketBuffer*>(rcast<u8*>(arg) + 0x204);

	if (wifiMode || !aid)
		Widescreen::enabled[aid] = pktBuf->buffers[aid][2];
	else
		Widescreen::enabled[aid] = Widescreen::enabled[0];

	VSConnectScene::syncInputSchemeWrapper(aid, scast<VSConnectScene*>(arg));
}

ncp_call(0x0215923C, 52)
void VSConnect_modifyCreatePacketBuffer(Net::PacketBuffer* pktBuf, u8 size, Net::OnPacketTransferComplete completeFunc, void* completeArg)
{
	pktBuf->create(size, completeFunc, completeArg); // Keep replaced instruction

	u8* dataToSend = rcast<u8*>(completeArg) + 0x21C;

	// Because of alignment, "dataToSend" still has 2 bytes free we
	// can use. For sending more we'd need to copy the data somewhere else
	dataToSend[2] = Widescreen::enabled[0];
}

ncp_repl(0x02159234, 52, "MOV R1, #3") // Bytes to send
ncp_over(0x0215933C, 52) const auto over_0215933C_52 = VSConnect_onPacketBufferReceived;
