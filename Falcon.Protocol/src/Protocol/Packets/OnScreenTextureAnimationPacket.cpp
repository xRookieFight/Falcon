#include "Protocol/Packets/OnScreenTextureAnimationPacket.h"

#include "Protocol/NetworkPacketHandler.h"

OnScreenTextureAnimationPacket::OnScreenTextureAnimationPacket()
        : mEffectId(0) {}

void OnScreenTextureAnimationPacket::write(BinaryStream &stream) const {
    stream.putLInt(mEffectId);
}

void OnScreenTextureAnimationPacket::read(ReadOnlyBinaryStream &stream) {
    mEffectId = stream.getLInt();
}

void OnScreenTextureAnimationPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
