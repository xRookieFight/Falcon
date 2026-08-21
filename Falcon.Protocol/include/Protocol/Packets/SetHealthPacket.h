#pragma once

#include "Protocol/Packet.h"

class SetHealthPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SetHealth;

    SetHealthPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SetHealthPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int32_t mHealth;
};
