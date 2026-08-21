#pragma once

#include "Protocol/Packet.h"

#include <vector>

class PlayerFogPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::PlayerFog;

    PlayerFogPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "PlayerFogPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    std::vector<std::string> mFogStack;
};
