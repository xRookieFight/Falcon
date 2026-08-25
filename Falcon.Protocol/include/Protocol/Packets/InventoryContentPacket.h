#pragma once

#include "protocol/Packet.h"
#include "protocol/types/FullContainerName.h"
#include "protocol/types/ItemStack.h"

#include <cstdint>
#include <vector>

class InventoryContentPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::InventoryContent;

    InventoryContentPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "InventoryContentPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    std::vector<ItemStack> mContents;
    int32_t mContainerId = 0;
    FullContainerName mContainerNameData;
    ItemStack mStorageItem;
};
