#pragma once

#include "Protocol/Packet.h"

#include <vector>

class EmoteListPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::EmoteList;

    EmoteListPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "EmoteListPacket"; }

    void write(BinaryStream &stream) const override;

    void read(ReadOnlyBinaryStream &stream) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    uint64_t mRuntimeEntityId;
    std::vector<UUID> mPieceIds;
};
