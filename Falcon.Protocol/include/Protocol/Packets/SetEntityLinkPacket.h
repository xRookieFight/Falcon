#pragma once

#include "Protocol/Packet.h"
#include "Protocol/Types/EntityLinkData.h"

class SetEntityLinkPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SetEntityLink;

    SetEntityLinkPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SetEntityLinkPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    EntityLinkData mEntityLink;
};
