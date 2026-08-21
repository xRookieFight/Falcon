#pragma once

#include "Protocol/Packet.h"

class CameraShakePacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::CameraShake;

    enum class ShakeType : int {
        Positional,
        Rotational
    };

    enum class ShakeAction : int {
        Add,
        Stop
    };

    CameraShakePacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "CameraShakePacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    float mIntensity;
    float mDuration;
    ShakeType mShakeType;
    ShakeAction mShakeAction;
};
