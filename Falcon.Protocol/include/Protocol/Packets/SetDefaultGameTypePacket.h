#pragma once

#include "Protocol/Packet.h"

class SetDefaultGameTypePacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SetDefaultGameType;

    SetDefaultGameTypePacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SetDefaultGameTypePacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int32_t mGamemode;
};
