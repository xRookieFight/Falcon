#include "Protocol/Packets/AnvilDamagePacket.h"

#include "Protocol/NetworkPacketHandler.h"

AnvilDamagePacket::AnvilDamagePacket() {}

void AnvilDamagePacket::write(BinaryStream &stream) const {
    stream.putBlockPosition(mPosition);
}

void AnvilDamagePacket::read(ReadOnlyBinaryStream &stream) {
    mPosition = stream.getBlockPosition();
}

void AnvilDamagePacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
