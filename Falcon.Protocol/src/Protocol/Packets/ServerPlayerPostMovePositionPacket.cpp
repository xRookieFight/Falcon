#include "Protocol/Packets/ServerPlayerPostMovePositionPacket.h"

#include "Protocol/NetworkPacketHandler.h"

ServerPlayerPostMovePositionPacket::ServerPlayerPostMovePositionPacket() {}

void ServerPlayerPostMovePositionPacket::write(BinaryStream &stream) const {
    stream.putVector3f(mPosition);
}

void ServerPlayerPostMovePositionPacket::read(ReadOnlyBinaryStream &stream) {
    mPosition = stream.getVector3f();
}

void ServerPlayerPostMovePositionPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
