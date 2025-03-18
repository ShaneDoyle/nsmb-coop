#include "Packet.hpp"

#include <nsmb/core/math.hpp>

constexpr u32 maxChunkSize = 252;

void Packet::transfer(u8 senderAid, void* sendBuffer, void* recvBuffer, u32 packetSize, Net::OnPacketTransferComplete completeFunc, void* completeArg)
{
	this->senderAid = senderAid;
	this->targetBuffer = rcast<u8*>(senderAid == Net::localAid ? sendBuffer : recvBuffer);
	this->packetSize = packetSize;
	this->completeFunc = completeFunc;
	this->completeArg = completeArg;

	this->bytesTransfered = 0;
	this->sequencerID = 0xFF;

	transferNextChunk();
}

void Packet::transferNextChunk()
{
	deleteSequencer();

	if (bytesTransfered >= packetSize)
	{
		// transfer complete
		completeFunc(senderAid, completeArg);
		return;
	}

	chunkSize = Math::min(maxChunkSize, packetSize - bytesTransfered);

	u8* chunkBuffer = targetBuffer + bytesTransfered;

	// consoleBuffers defines where writePacketSequencer will write the bytes on the receiving console
	// we can leave anything but the sender undefined, they won't be used
	u8* consoleBuffers[4];
	consoleBuffers[senderAid] = chunkBuffer;

	sequencerID = Net::Core::createPacketSequencer(consoleBuffers, chunkSize, chunkTransferComplete, this);

	if (senderAid == Net::localAid && Net::isConnected())
	{
		Net::Core::writePacketSequencer(sequencerID, chunkSize, chunkBuffer);
	}

	bytesTransfered += chunkSize;
}

void Packet::chunkTransferComplete(u16 aid, void* arg)
{
	Packet* self = rcast<Packet*>(arg);

	self->transferNextChunk();
}

void Packet::deleteSequencer()
{
	if (sequencerID != 0xFF)
		Net::Core::deletePacketSequencer(sequencerID);
}
