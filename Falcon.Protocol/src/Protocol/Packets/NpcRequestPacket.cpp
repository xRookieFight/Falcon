#include "Protocol/Packets/NpcRequestPacket.h"

#include "Protocol/NetworkPacketHandler.h"

NpcRequestPacket::NpcRequestPacket()
        : mRuntimeEntityId(0), mRequestType(RequestType::SetAction), mActionType(0) {}

void NpcRequestPacket::write(BinaryStream &stream) const {
    stream.putUnsignedVarLong(mRuntimeEntityId);
    stream.putByte((unsigned char) mRequestType);
    stream.putString(mCommand);
    stream.putByte((unsigned char) mActionType);
    stream.putString(mSceneName);
}

void NpcRequestPacket::read(ReadOnlyBinaryStream &stream) {
    mRuntimeEntityId = stream.getUnsignedVarLong();
    mRequestType = (RequestType) stream.getByte();
    mCommand = stream.getString();
    mActionType = stream.getByte();
    mSceneName = stream.getString();
}

void NpcRequestPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
