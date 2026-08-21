#include "Protocol/Packets/ScriptMessagePacket.h"

#include "Protocol/NetworkPacketHandler.h"

ScriptMessagePacket::ScriptMessagePacket() {}

void ScriptMessagePacket::write(BinaryStream &stream) const {
    stream.putString(mChannel);
    stream.putString(mMessage);
}

void ScriptMessagePacket::read(ReadOnlyBinaryStream &stream) {
    mChannel = stream.getString();
    mMessage = stream.getString();
}

void ScriptMessagePacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
