#pragma once

#include "Protocol/Packet.h"

class DebugInfoPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::DebugInfo;

    DebugInfoPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "DebugInfoPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int64_t mUniqueEntityId;
    std::string mData;
};
