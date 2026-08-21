#pragma once

#include "Protocol/Packet.h"

class ShowCreditsPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::ShowCredits;

    enum class Status : int {
        StartCredits,
        EndCredits
    };

    ShowCreditsPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "ShowCreditsPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    uint64_t mRuntimeEntityId;
    Status mStatus;
};
