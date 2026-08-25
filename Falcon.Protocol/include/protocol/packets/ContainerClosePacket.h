#pragma once

#include "protocol/Packet.h"
#include "protocol/types/ContainerType.h"

#include <cstdint>

class ContainerClosePacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::ContainerClose;

    ContainerClosePacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "ContainerClosePacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int8_t mWindowId = 0;
    ContainerType mType = ContainerType::None;
    bool mServerInitiated = false;
};
