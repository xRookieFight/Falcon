#include "protocol/packets/ServerSettingsRequestPacket.h"

#include "protocol/NetworkPacketHandler.h"

ServerSettingsRequestPacket::ServerSettingsRequestPacket() {}

void ServerSettingsRequestPacket::write(BinaryStream &, const PacketCodecContext &) const {
}

void ServerSettingsRequestPacket::read(ReadOnlyBinaryStream &, const PacketCodecContext &) {
}

void ServerSettingsRequestPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
