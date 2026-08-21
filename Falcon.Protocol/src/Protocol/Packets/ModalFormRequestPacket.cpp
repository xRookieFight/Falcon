#include "Protocol/Packets/ModalFormRequestPacket.h"

#include "Protocol/NetworkPacketHandler.h"

ModalFormRequestPacket::ModalFormRequestPacket()
        : mFormId(0) {}

void ModalFormRequestPacket::write(BinaryStream &stream) const {
    stream.putUnsignedVarInt(mFormId);
    stream.putString(mFormData);
}

void ModalFormRequestPacket::read(ReadOnlyBinaryStream &stream) {
    mFormId = stream.getUnsignedVarInt();
    mFormData = stream.getString();
}

void ModalFormRequestPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
