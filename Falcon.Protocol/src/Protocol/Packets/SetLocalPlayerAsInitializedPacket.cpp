#include "Protocol/Packets/SetLocalPlayerAsInitializedPacket.h"

#include "Protocol/NetworkPacketHandler.h"

SetLocalPlayerAsInitializedPacket::SetLocalPlayerAsInitializedPacket()
        : mRuntimeEntityId(0) {}

void SetLocalPlayerAsInitializedPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putUnsignedVarLong(mRuntimeEntityId);
}

void SetLocalPlayerAsInitializedPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mRuntimeEntityId = stream.getUnsignedVarLong();
}

void SetLocalPlayerAsInitializedPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
