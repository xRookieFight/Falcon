#include "Protocol/Packets/TakeItemEntityPacket.h"

#include "Protocol/NetworkPacketHandler.h"

TakeItemEntityPacket::TakeItemEntityPacket()
        : mItemRuntimeEntityId(0), mRuntimeEntityId(0) {}

void TakeItemEntityPacket::write(BinaryStream &stream) const {
    stream.putUnsignedVarLong(mItemRuntimeEntityId);
    stream.putUnsignedVarLong(mRuntimeEntityId);
}

void TakeItemEntityPacket::read(ReadOnlyBinaryStream &stream) {
    mItemRuntimeEntityId = stream.getUnsignedVarLong();
    mRuntimeEntityId = stream.getUnsignedVarLong();
}

void TakeItemEntityPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
