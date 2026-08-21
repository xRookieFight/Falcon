#pragma once

#include "Protocol/Packet.h"

class ClientCacheStatusPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::ClientCacheStatus;

    ClientCacheStatusPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "ClientCacheStatusPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    bool mSupported;
};
