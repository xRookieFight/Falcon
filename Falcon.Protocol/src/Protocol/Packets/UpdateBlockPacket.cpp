#include "Protocol/Packets/UpdateBlockPacket.h"

#include "Protocol/NetworkPacketHandler.h"

UpdateBlockPacket::UpdateBlockPacket()
        : mRuntimeId(0), mFlags(Flag::All), mDataLayer(0) {}

void UpdateBlockPacket::write(BinaryStream &stream) const {
    stream.putBlockPosition(mBlockPosition);
    stream.putUnsignedVarInt(mRuntimeId);
    stream.putUnsignedVarInt(mFlags);
    stream.putUnsignedVarInt(mDataLayer);
}

void UpdateBlockPacket::read(ReadOnlyBinaryStream &stream) {
    mBlockPosition = stream.getBlockPosition();
    mRuntimeId = stream.getUnsignedVarInt();
    mFlags = stream.getUnsignedVarInt();
    mDataLayer = stream.getUnsignedVarInt();
}

void UpdateBlockPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
