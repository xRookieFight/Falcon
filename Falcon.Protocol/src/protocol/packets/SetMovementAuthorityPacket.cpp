#include "protocol/packets/SetMovementAuthorityPacket.h"

#include "protocol/NetworkPacketHandler.h"

SetMovementAuthorityPacket::SetMovementAuthorityPacket()
        : mMode(AuthoritativeMovementMode::Server) {}

void SetMovementAuthorityPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putByte((unsigned char) mMode);
}

void SetMovementAuthorityPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mMode = (AuthoritativeMovementMode) stream.getByte();
}

void SetMovementAuthorityPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    (void) id;
    (void) handler;
}
