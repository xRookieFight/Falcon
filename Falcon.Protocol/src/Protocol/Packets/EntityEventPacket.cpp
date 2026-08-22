#include "Protocol/Packets/EntityEventPacket.h"

#include "Protocol/NetworkPacketHandler.h"

EntityEventPacket::EntityEventPacket() = default;

void EntityEventPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putUnsignedVarLong(mRuntimeEntityId);
    stream.putByte(mEventId);
    stream.putVarInt(mEventData);

    stream.putOptionalPresent(mHasFirePosition);
    if (mHasFirePosition)
        stream.putVector3f(mFirePosition);
}

void EntityEventPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mRuntimeEntityId = stream.getUnsignedVarLong();
    mEventId = stream.getByte();
    mEventData = stream.getVarInt();

    mHasFirePosition = stream.getBool();
    if (mHasFirePosition)
        mFirePosition = stream.getVector3f();
}

void EntityEventPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
