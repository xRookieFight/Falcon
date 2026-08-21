#pragma once

#include "Protocol/Packet.h"

class SetEntityMotionPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SetEntityMotion;

    SetEntityMotionPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SetEntityMotionPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    uint64_t mRuntimeEntityId;
    Vector3f mMotion;
    uint64_t mTick;
};
