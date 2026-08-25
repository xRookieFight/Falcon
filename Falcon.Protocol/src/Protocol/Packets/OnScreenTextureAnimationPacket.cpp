#include "protocol/packets/OnScreenTextureAnimationPacket.h"

#include "protocol/NetworkPacketHandler.h"

OnScreenTextureAnimationPacket::OnScreenTextureAnimationPacket()
        : mEffectId(0) {}

void OnScreenTextureAnimationPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putLInt(mEffectId);
}

void OnScreenTextureAnimationPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mEffectId = stream.getLInt();
}

void OnScreenTextureAnimationPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
