#include "Protocol/Packets/ShowProfilePacket.h"

#include "Protocol/NetworkPacketHandler.h"

ShowProfilePacket::ShowProfilePacket() {}

void ShowProfilePacket::write(BinaryStream &stream) const {
    stream.putString(mXuid);
}

void ShowProfilePacket::read(ReadOnlyBinaryStream &stream) {
    mXuid = stream.getString();
}

void ShowProfilePacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
