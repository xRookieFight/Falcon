#pragma once

#include "core/nbt/Tag.h"
#include "protocol/Packet.h"

class SyncActorPropertyPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SyncEntityProperty;

    SyncActorPropertyPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SyncActorPropertyPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    Tag mData;
};
