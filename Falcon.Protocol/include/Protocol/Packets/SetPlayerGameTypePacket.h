#pragma once

#include "Protocol/Packet.h"

class SetPlayerGameTypePacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SetPlayerGameType;

    SetPlayerGameTypePacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SetPlayerGameTypePacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int32_t mGamemode;
};
