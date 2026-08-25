#include "protocol/packets/MapInfoRequestPacket.h"

#include "protocol/NetworkPacketHandler.h"

MapInfoRequestPacket::MapInfoRequestPacket() = default;

void MapInfoRequestPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putVarLong(mUniqueMapId);

    stream.putUnsignedVarInt((uint32_t) mPixels.size());
    for (const MapPixel &pixel : mPixels) {
        stream.putLInt((uint32_t) pixel.mPixel);
        stream.putLShort((uint16_t) pixel.mIndex);
    }
}

void MapInfoRequestPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mUniqueMapId = stream.getVarLong();

    uint32_t count = stream.getUnsignedVarInt();
    mPixels.clear();
    mPixels.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        MapPixel pixel;
        pixel.mPixel = (int32_t) stream.getLInt();
        pixel.mIndex = stream.getLShort();
        mPixels.push_back(pixel);
    }
}

void MapInfoRequestPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
