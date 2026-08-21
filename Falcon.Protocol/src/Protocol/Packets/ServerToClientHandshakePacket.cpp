#include "Protocol/Packets/ServerToClientHandshakePacket.h"

#include "Protocol/NetworkPacketHandler.h"

ServerToClientHandshakePacket::ServerToClientHandshakePacket() {}

void ServerToClientHandshakePacket::write(BinaryStream &stream) const {
    stream.putString(mJwt);
}

void ServerToClientHandshakePacket::read(ReadOnlyBinaryStream &stream) {
    mJwt = stream.getString();
}

void ServerToClientHandshakePacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
