#include "protocol/packets/ServerToClientHandshakePacket.h"

#include "protocol/NetworkPacketHandler.h"

ServerToClientHandshakePacket::ServerToClientHandshakePacket() {}

void ServerToClientHandshakePacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putString(mJwt);
}

void ServerToClientHandshakePacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mJwt = stream.getString();
}

void ServerToClientHandshakePacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
