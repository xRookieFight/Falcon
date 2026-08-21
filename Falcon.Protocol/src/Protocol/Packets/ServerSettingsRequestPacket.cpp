#include "Protocol/Packets/ServerSettingsRequestPacket.h"

#include "Protocol/NetworkPacketHandler.h"

ServerSettingsRequestPacket::ServerSettingsRequestPacket() {}

void ServerSettingsRequestPacket::write(BinaryStream &) const {
}

void ServerSettingsRequestPacket::read(ReadOnlyBinaryStream &) {
}

void ServerSettingsRequestPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
