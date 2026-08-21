#pragma once

#include "Protocol/Packet.h"

class RemoveEntityPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::RemoveEntity;

    RemoveEntityPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "RemoveEntityPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int64_t mUniqueEntityId;
};
