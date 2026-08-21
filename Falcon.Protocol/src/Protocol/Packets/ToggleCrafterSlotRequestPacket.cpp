#include "Protocol/Packets/ToggleCrafterSlotRequestPacket.h"

#include "Protocol/NetworkPacketHandler.h"

ToggleCrafterSlotRequestPacket::ToggleCrafterSlotRequestPacket() = default;

void ToggleCrafterSlotRequestPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putLInt((uint32_t) mBlockPosition.x);
    stream.putLInt((uint32_t) mBlockPosition.y);
    stream.putLInt((uint32_t) mBlockPosition.z);
    stream.putByte((unsigned char) mSlot);
    stream.putBool(mDisabled);
}

void ToggleCrafterSlotRequestPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    int32_t x = (int32_t) stream.getLInt();
    int32_t y = (int32_t) stream.getLInt();
    int32_t z = (int32_t) stream.getLInt();
    mBlockPosition = Vector3i(x, y, z);
    mSlot = (int8_t) stream.getSignedByte();
    mDisabled = stream.getBool();
}

void ToggleCrafterSlotRequestPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
