#pragma once

#include "Core/Math/Vector3f.h"
#include "Protocol/Packet.h"

#include <cstdint>

class MoveEntityAbsolutePacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::MoveEntityAbsolute;

    MoveEntityAbsolutePacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "MoveEntityAbsolutePacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int64_t mRuntimeEntityId = 0;
    Vector3f mPosition;
    Vector3f mRotation;
    bool mOnGround = false;
    bool mTeleported = false;
    bool mForceMove = false;
    bool mForceCompletion = false;
};
