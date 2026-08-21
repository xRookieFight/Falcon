#include "Protocol/Packets/RespawnPacket.h"

#include "Protocol/NetworkPacketHandler.h"

RespawnPacket::RespawnPacket()
        : mState(State::ServerSearching), mRuntimeEntityId(0) {}

void RespawnPacket::write(BinaryStream &stream) const {
    stream.putVector3f(mPosition);
    stream.putByte((unsigned char) mState);
    stream.putUnsignedVarLong(mRuntimeEntityId);
}

void RespawnPacket::read(ReadOnlyBinaryStream &stream) {
    mPosition = stream.getVector3f();
    mState = (State) stream.getByte();
    mRuntimeEntityId = stream.getUnsignedVarLong();
}

void RespawnPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
