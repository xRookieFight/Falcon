#pragma once

#include "Protocol/Packet.h"

class SetTimePacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SetTime;

    SetTimePacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SetTimePacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int32_t mTime;
};
