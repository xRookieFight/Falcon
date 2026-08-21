#include "Protocol/Packets/ToastRequestPacket.h"

#include "Protocol/NetworkPacketHandler.h"

ToastRequestPacket::ToastRequestPacket() {}

void ToastRequestPacket::write(BinaryStream &stream) const {
    stream.putString(mTitle);
    stream.putString(mContent);
}

void ToastRequestPacket::read(ReadOnlyBinaryStream &stream) {
    mTitle = stream.getString();
    mContent = stream.getString();
}

void ToastRequestPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
