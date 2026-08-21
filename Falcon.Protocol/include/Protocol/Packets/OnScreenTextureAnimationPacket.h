#pragma once

#include "Protocol/Packet.h"

class OnScreenTextureAnimationPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::OnScreenTextureAnimation;

    OnScreenTextureAnimationPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "OnScreenTextureAnimationPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    uint32_t mEffectId;
};
