#pragma once

#include "Protocol/Packet.h"

class TickingAreasLoadStatusPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::TickingAreasLoadStatus;

    TickingAreasLoadStatusPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "TickingAreasLoadStatusPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    bool mWaitingForPreload;
};
