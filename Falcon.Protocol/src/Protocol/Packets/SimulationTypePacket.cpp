#include "protocol/packets/SimulationTypePacket.h"

#include "protocol/NetworkPacketHandler.h"

SimulationTypePacket::SimulationTypePacket()
        : mType(Type::Game) {}

void SimulationTypePacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putByte((unsigned char) mType);
}

void SimulationTypePacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mType = (Type) stream.getByte();
}

void SimulationTypePacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
