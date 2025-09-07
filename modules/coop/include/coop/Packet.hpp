#pragma once

#include <nsmb/core/net.hpp>

class Packet
{
public:
	// Note: Packet size must be 4-byte aligned
	void transfer(u8 senderAid, void* sendBuffer, void* recvBuffer, u32 packetSize, Net::OnPacketTransferComplete completeFunc, void* completeArg = nullptr);

private:
	void transferNextChunk();
	static void chunkTransferComplete(u16 aid, void* arg);
	void deleteSequencer();

	u8 senderAid;
	u32 packetSize;
	Net::PacketSequencer sequencer;
	u8 sequencerID;
	u32 chunkSize;
	u32 bytesTransfered;
	u8* targetBuffer;
	Net::OnPacketTransferComplete completeFunc;
	void* completeArg;
};
