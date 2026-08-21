#include "Protocol/Packets/SetEntityLinkPacket.h"

#include "Protocol/EntityCodec.h"
#include "Protocol/NetworkPacketHandler.h"

SetEntityLinkPacket::SetEntityLinkPacket() = default;

void SetEntityLinkPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    EntityCodec::writeEntityLink(stream, mEntityLink);
}

void SetEntityLinkPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mEntityLink = EntityCodec::readEntityLink(stream);
}

void SetEntityLinkPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
