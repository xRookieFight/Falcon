#pragma once

#include "core/math/Vector3i.h"
#include "protocol/Packet.h"
#include "protocol/types/SubChunkData.h"

#include <vector>

class SubChunkPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SubChunk;

    SubChunkPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SubChunkPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    bool mCacheEnabled;
    int32_t mDimension;
    Vector3i mCenterPosition;
    std::vector<SubChunkData> mSubChunks;
};
