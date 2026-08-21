#pragma once

#include "Protocol/Packet.h"
#include "Protocol/Types/DebugShapeData.h"

#include <vector>

class DebugDrawerPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::DebugDrawer;

    DebugDrawerPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "DebugDrawerPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    std::vector<DebugShapeData> mShapes;
};
