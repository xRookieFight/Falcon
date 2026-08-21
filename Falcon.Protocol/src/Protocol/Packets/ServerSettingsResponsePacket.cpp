#include "Protocol/Packets/ServerSettingsResponsePacket.h"

#include "Protocol/NetworkPacketHandler.h"

ServerSettingsResponsePacket::ServerSettingsResponsePacket()
        : mFormId(0) {}

void ServerSettingsResponsePacket::write(BinaryStream &stream) const {
    stream.putUnsignedVarInt(mFormId);
    stream.putString(mFormData);
}

void ServerSettingsResponsePacket::read(ReadOnlyBinaryStream &stream) {
    mFormId = stream.getUnsignedVarInt();
    mFormData = stream.getString();
}

void ServerSettingsResponsePacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
