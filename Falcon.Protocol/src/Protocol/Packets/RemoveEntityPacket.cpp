#include "Protocol/Packets/RemoveEntityPacket.h"

#include "Protocol/NetworkPacketHandler.h"

RemoveEntityPacket::RemoveEntityPacket()
        : mUniqueActorId(0) {}

void RemoveEntityPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putVarLong(mUniqueActorId);
}

void RemoveEntityPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mUniqueActorId = stream.getVarLong();
}

void RemoveEntityPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
