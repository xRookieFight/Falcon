#pragma once

#include "protocol/Packet.h"
#include "protocol/types/ItemStackRequest.h"

#include <vector>

class ItemStackRequestPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::ItemStackRequest;

    ItemStackRequestPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "ItemStackRequestPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    std::vector<ItemStackRequest> mRequests;
};
