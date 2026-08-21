#pragma once

#include "Protocol/Packet.h"

class SetLocalPlayerAsInitializedPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SetLocalPlayerAsInitialized;

    SetLocalPlayerAsInitializedPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SetLocalPlayerAsInitializedPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    uint64_t mRuntimeEntityId;
};
