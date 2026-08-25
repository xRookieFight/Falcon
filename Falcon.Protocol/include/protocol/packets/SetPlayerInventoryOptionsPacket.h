#pragma once

#include "protocol/Packet.h"
#include "protocol/types/InventoryLayout.h"
#include "protocol/types/InventoryTabLeft.h"
#include "protocol/types/InventoryTabRight.h"

class SetPlayerInventoryOptionsPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::SetPlayerInventoryOptions;

    SetPlayerInventoryOptionsPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "SetPlayerInventoryOptionsPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    InventoryTabLeft mLeftTab = InventoryTabLeft::None;
    InventoryTabRight mRightTab = InventoryTabRight::None;
    bool mFiltering = false;
    InventoryLayout mLayout = InventoryLayout::None;
    InventoryLayout mCraftingLayout = InventoryLayout::None;
};
