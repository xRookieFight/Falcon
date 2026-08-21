#include "Protocol/Packets/SetCommandsEnabledPacket.h"

#include "Protocol/NetworkPacketHandler.h"

SetCommandsEnabledPacket::SetCommandsEnabledPacket()
        : mCommandsEnabled(false) {}

void SetCommandsEnabledPacket::write(BinaryStream &stream) const {
    stream.putBool(mCommandsEnabled);
}

void SetCommandsEnabledPacket::read(ReadOnlyBinaryStream &stream) {
    mCommandsEnabled = stream.getBool();
}

void SetCommandsEnabledPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
