#include "Protocol/Packets/ClientboundCloseFormPacket.h"

#include "Protocol/NetworkPacketHandler.h"

ClientboundCloseFormPacket::ClientboundCloseFormPacket() {}

void ClientboundCloseFormPacket::write(BinaryStream &) const {
}

void ClientboundCloseFormPacket::read(ReadOnlyBinaryStream &) {
}

void ClientboundCloseFormPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
