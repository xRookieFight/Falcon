#pragma once

#include "Protocol/Packet.h"

class MobEffectPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::MobEffect;

    enum class Event : int {
        None,
        Add,
        Modify,
        Remove
    };

    MobEffectPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "MobEffectPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    uint64_t mRuntimeEntityId;
    Event mEvent;
    int32_t mEffectId;
    int32_t mAmplifier;
    bool mParticles;
    int32_t mDuration;
    uint64_t mTick;
    bool mAmbient;
};
