#include "Protocol/Packets/TakeItemEntityPacket.h"

#include "Protocol/NetworkPacketHandler.h"

TakeItemEntityPacket::TakeItemEntityPacket()
        : mItemRuntimeActorId(0), mRuntimeActorId(0) {}

void TakeItemEntityPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putUnsignedVarLong(mItemRuntimeActorId);
    stream.putUnsignedVarLong(mRuntimeActorId);
}

void TakeItemEntityPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mItemRuntimeActorId = stream.getUnsignedVarLong();
    mRuntimeActorId = stream.getUnsignedVarLong();
}

void TakeItemEntityPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
