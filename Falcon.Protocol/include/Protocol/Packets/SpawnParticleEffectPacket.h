#pragma once

#include "Protocol/Packet.h"

class SpawnParticleEffectPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SpawnParticleEffect;

    SpawnParticleEffectPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SpawnParticleEffectPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int32_t mDimensionId;
    int64_t mUniqueEntityId;
    Vector3f mPosition;
    std::string mIdentifier;
    bool mHasMolangVariablesJson;
    std::string mMolangVariablesJson;
};
