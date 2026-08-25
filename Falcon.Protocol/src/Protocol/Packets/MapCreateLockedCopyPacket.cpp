#include "protocol/packets/MapCreateLockedCopyPacket.h"

#include "protocol/NetworkPacketHandler.h"

MapCreateLockedCopyPacket::MapCreateLockedCopyPacket()
        : mOriginalMapId(0), mNewMapId(0) {}

void MapCreateLockedCopyPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putVarLong(mOriginalMapId);
    stream.putVarLong(mNewMapId);
}

void MapCreateLockedCopyPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mOriginalMapId = stream.getVarLong();
    mNewMapId = stream.getVarLong();
}

void MapCreateLockedCopyPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
