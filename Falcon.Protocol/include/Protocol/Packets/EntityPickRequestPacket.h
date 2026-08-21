#pragma once

#include "Protocol/Packet.h"

class EntityPickRequestPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::EntityPickRequest;

    EntityPickRequestPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "EntityPickRequestPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    uint64_t mRuntimeEntityId;
    int32_t mHotbarSlot;
    bool mWithData;
};
