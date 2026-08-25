#pragma once

#include "core/math/Vector3f.h"
#include "protocol/Packet.h"

class LevelEventPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::LevelEvent;

    enum Event : int32_t {
        ParticlePunchBlock = 2014,
        BlockStartBreak = 3600,
        BlockStopBreak = 3601,
        BlockUpdateBreak = 3602,
        ParticleDestroy = 2001
    };

    LevelEventPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "LevelEventPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int32_t mEventId;
    Vector3f mPosition;
    int32_t mData;
};
