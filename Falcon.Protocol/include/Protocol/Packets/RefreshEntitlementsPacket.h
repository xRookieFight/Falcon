#pragma once

#include "Protocol/Packet.h"

class RefreshEntitlementsPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::RefreshEntitlements;

    RefreshEntitlementsPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "RefreshEntitlementsPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;
};
