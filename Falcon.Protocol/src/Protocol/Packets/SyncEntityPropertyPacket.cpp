#include "Protocol/Packets/SyncEntityPropertyPacket.h"

#include "Core/NBT/NbtIo.h"
#include "Protocol/NetworkPacketHandler.h"

SyncEntityPropertyPacket::SyncEntityPropertyPacket()
        : mData(Tag::ofCompound()) {}

void SyncEntityPropertyPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    NbtIo::writeTag(stream, mData, NbtVariant::Network);
}

void SyncEntityPropertyPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mData = NbtIo::readTag(stream, NbtVariant::Network);
}

void SyncEntityPropertyPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
