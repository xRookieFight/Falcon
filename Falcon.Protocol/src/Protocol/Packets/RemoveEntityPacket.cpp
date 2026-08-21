#include "Protocol/Packets/RemoveEntityPacket.h"

#include "Protocol/NetworkPacketHandler.h"

RemoveEntityPacket::RemoveEntityPacket()
        : mUniqueEntityId(0) {}

void RemoveEntityPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putVarLong(mUniqueEntityId);
}

void RemoveEntityPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mUniqueEntityId = stream.getVarLong();
}

void RemoveEntityPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
