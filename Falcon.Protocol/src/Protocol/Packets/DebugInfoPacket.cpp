#include "Protocol/Packets/DebugInfoPacket.h"

#include "Protocol/NetworkPacketHandler.h"

DebugInfoPacket::DebugInfoPacket()
        : mUniqueEntityId(0) {}

void DebugInfoPacket::write(BinaryStream &stream) const {
    stream.putVarLong(mUniqueEntityId);
    stream.putString(mData);
}

void DebugInfoPacket::read(ReadOnlyBinaryStream &stream) {
    mUniqueEntityId = stream.getVarLong();
    mData = stream.getString();
}

void DebugInfoPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
