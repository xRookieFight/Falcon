#pragma once

#include "protocol/Packet.h"
#include "protocol/types/DebugShapeData.h"

#include <vector>

class PrimitiveShapesPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::DebugDrawer;

    PrimitiveShapesPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "PrimitiveShapesPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    std::vector<DebugShapeData> mShapes;
};
