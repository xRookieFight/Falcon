#include "Protocol/Packets/AvailableEntityIdentifiersPacket.h"

#include "Core/NBT/NbtIo.h"
#include "Protocol/NetworkPacketHandler.h"

AvailableEntityIdentifiersPacket::AvailableEntityIdentifiersPacket() = default;

void AvailableEntityIdentifiersPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    NbtIo::writeTag(stream, mIdentifiers, NbtVariant::Network);
}

void AvailableEntityIdentifiersPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mIdentifiers = NbtIo::readTag(stream, NbtVariant::Network);
}

void AvailableEntityIdentifiersPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
