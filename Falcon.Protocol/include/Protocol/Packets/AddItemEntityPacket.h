#pragma once

#include "Core/Math/Vector3f.h"
#include "Protocol/Packet.h"
#include "Protocol/Types/EntityDataMap.h"
#include "Protocol/Types/ItemStack.h"

#include <cstdint>

class AddItemEntityPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::AddItemEntity;

    AddItemEntityPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "AddItemEntityPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int64_t mUniqueEntityId = 0;
    uint64_t mRuntimeEntityId = 0;
    ItemStack mItemInHand;
    Vector3f mPosition;
    Vector3f mMotion;
    EntityDataMap mMetadata;
    bool mFromFishing = false;
};
