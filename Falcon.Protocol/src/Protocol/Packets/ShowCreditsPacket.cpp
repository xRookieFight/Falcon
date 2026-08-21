#include "Protocol/Packets/ShowCreditsPacket.h"

#include "Protocol/NetworkPacketHandler.h"

ShowCreditsPacket::ShowCreditsPacket()
        : mRuntimeEntityId(0), mStatus(Status::StartCredits) {}

void ShowCreditsPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putUnsignedVarLong(mRuntimeEntityId);
    stream.putVarInt((int32_t) mStatus);
}

void ShowCreditsPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mRuntimeEntityId = stream.getUnsignedVarLong();
    mStatus = (Status) stream.getVarInt();
}

void ShowCreditsPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
