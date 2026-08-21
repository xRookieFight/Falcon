#pragma once

#include "Protocol/Packet.h"

class RequestChunkRadiusPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::RequestChunkRadius;

    RequestChunkRadiusPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "RequestChunkRadiusPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int32_t mRadius;
    int32_t mMaxRadius;
};
