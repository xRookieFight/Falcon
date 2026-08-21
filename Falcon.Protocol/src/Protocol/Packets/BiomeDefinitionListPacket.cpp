#include "Protocol/Packets/BiomeDefinitionListPacket.h"

#include "Core/NBT/NbtIo.h"
#include "Protocol/NetworkPacketHandler.h"

BiomeDefinitionListPacket::BiomeDefinitionListPacket() = default;

void BiomeDefinitionListPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    NbtIo::writeTag(stream, mDefinitions, NbtVariant::Network);
}

void BiomeDefinitionListPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mDefinitions = NbtIo::readTag(stream, NbtVariant::Network);
}

void BiomeDefinitionListPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
