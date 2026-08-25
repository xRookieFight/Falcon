#pragma once

#include "protocol/Packet.h"
#include "protocol/types/TrimMaterial.h"
#include "protocol/types/TrimPattern.h"

#include <vector>

class TrimDataPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::TrimData;

    TrimDataPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "TrimDataPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    std::vector<TrimPattern> mPatterns;
    std::vector<TrimMaterial> mMaterials;
};
