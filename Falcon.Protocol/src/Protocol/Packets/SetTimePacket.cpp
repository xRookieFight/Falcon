#include "Protocol/Packets/SetTimePacket.h"

#include "Protocol/NetworkPacketHandler.h"

SetTimePacket::SetTimePacket()
        : mTime(0) {}

void SetTimePacket::write(BinaryStream &stream) const {
    stream.putVarInt(mTime);
}

void SetTimePacket::read(ReadOnlyBinaryStream &stream) {
    mTime = stream.getVarInt();
}

void SetTimePacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
