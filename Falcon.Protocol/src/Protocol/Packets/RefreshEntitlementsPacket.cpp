#include "Protocol/Packets/RefreshEntitlementsPacket.h"

#include "Protocol/NetworkPacketHandler.h"

RefreshEntitlementsPacket::RefreshEntitlementsPacket() {}

void RefreshEntitlementsPacket::write(BinaryStream &) const {
}

void RefreshEntitlementsPacket::read(ReadOnlyBinaryStream &) {
}

void RefreshEntitlementsPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
