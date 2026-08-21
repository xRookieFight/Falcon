#include "Protocol/Packets/ChunkRadiusUpdatedPacket.h"

#include "Protocol/NetworkPacketHandler.h"

ChunkRadiusUpdatedPacket::ChunkRadiusUpdatedPacket()
        : mRadius(0) {}

void ChunkRadiusUpdatedPacket::write(BinaryStream &stream) const {
    stream.putVarInt(mRadius);
}

void ChunkRadiusUpdatedPacket::read(ReadOnlyBinaryStream &stream) {
    mRadius = stream.getVarInt();
}

void ChunkRadiusUpdatedPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
