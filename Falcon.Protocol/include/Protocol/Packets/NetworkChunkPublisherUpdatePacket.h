#pragma once

#include "Core/Math/Vector3i.h"
#include "Protocol/Packet.h"

#include <vector>

class NetworkChunkPublisherUpdatePacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::NetworkChunkPublisherUpdate;

    NetworkChunkPublisherUpdatePacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "NetworkChunkPublisherUpdatePacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    Vector3i mPosition;
    uint32_t mRadius;
    std::vector<Vector2i> mSavedChunks;
};
