#include "protocol/packets/RefreshEntitlementsPacket.h"

#include "protocol/NetworkPacketHandler.h"

RefreshEntitlementsPacket::RefreshEntitlementsPacket() {}

void RefreshEntitlementsPacket::write(BinaryStream &, const PacketCodecContext &) const {
}

void RefreshEntitlementsPacket::read(ReadOnlyBinaryStream &, const PacketCodecContext &) {
}

void RefreshEntitlementsPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
