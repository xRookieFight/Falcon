#include "Protocol/Packets/SetEntityMotionPacket.h"

#include "Protocol/NetworkPacketHandler.h"

SetEntityMotionPacket::SetEntityMotionPacket()
        : mRuntimeEntityId(0), mTick(0) {}

void SetEntityMotionPacket::write(BinaryStream &stream) const {
    stream.putUnsignedVarLong(mRuntimeEntityId);
    stream.putVector3f(mMotion);
    stream.putUnsignedVarLong(mTick);
}

void SetEntityMotionPacket::read(ReadOnlyBinaryStream &stream) {
    mRuntimeEntityId = stream.getUnsignedVarLong();
    mMotion = stream.getVector3f();
    mTick = stream.getUnsignedVarLong();
}

void SetEntityMotionPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
