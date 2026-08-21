#include "Protocol/Packets/EntityPickRequestPacket.h"

#include "Protocol/NetworkPacketHandler.h"

EntityPickRequestPacket::EntityPickRequestPacket()
        : mRuntimeEntityId(0), mHotbarSlot(0), mWithData(false) {}

void EntityPickRequestPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putLLong(mRuntimeEntityId);
    stream.putByte((unsigned char) mHotbarSlot);
    stream.putBool(mWithData);
}

void EntityPickRequestPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mRuntimeEntityId = stream.getLLong();
    mHotbarSlot = stream.getByte();
    mWithData = stream.getBool();
}

void EntityPickRequestPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
