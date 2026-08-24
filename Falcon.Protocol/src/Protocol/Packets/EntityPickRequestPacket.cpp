#include "Protocol/Packets/EntityPickRequestPacket.h"

#include "Protocol/NetworkPacketHandler.h"

EntityPickRequestPacket::EntityPickRequestPacket()
        : mRuntimeActorId(0), mHotbarSlot(0), mWithData(false) {}

void EntityPickRequestPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putLLong(mRuntimeActorId);
    stream.putByte((unsigned char) mHotbarSlot);
    stream.putBool(mWithData);
}

void EntityPickRequestPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mRuntimeActorId = stream.getLLong();
    mHotbarSlot = stream.getByte();
    mWithData = stream.getBool();
}

void EntityPickRequestPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
