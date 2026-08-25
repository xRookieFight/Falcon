#pragma once

#include "protocol/Packet.h"
#include "protocol/types/EnchantOptionData.h"

#include <vector>

class PlayerEnchantOptionsPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::PlayerEnchantOptions;

    PlayerEnchantOptionsPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "PlayerEnchantOptionsPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    std::vector<EnchantOptionData> mOptions;
};
