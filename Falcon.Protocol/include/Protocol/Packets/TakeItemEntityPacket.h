#pragma once

#include "Protocol/Packet.h"

class TakeItemEntityPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::TakeItemEntity;

    TakeItemEntityPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "TakeItemEntityPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    uint64_t mItemRuntimeEntityId;
    uint64_t mRuntimeEntityId;
};
