#pragma once

#include "protocol/Packet.h"
#include "protocol/types/CraftingRecipeEntry.h"

#include <vector>

class CraftingDataPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::CraftingData;

    CraftingDataPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "CraftingDataPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    std::vector<CraftingRecipeEntry> mShapedRecipes;
    std::vector<CraftingRecipeEntry> mShapelessRecipes;
    bool mCleanRecipes = false;
};
