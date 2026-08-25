#pragma once

#include "protocol/Packet.h"
#include "protocol/types/InventoryActionData.h"
#include "protocol/types/InventoryTransactionType.h"
#include "protocol/types/ItemStack.h"
#include "protocol/types/ItemUseTransaction.h"
#include "protocol/types/LegacySetItemSlotData.h"

#include "core/math/Vector3f.h"
#include "core/math/Vector3i.h"

#include <cstdint>
#include <memory>
#include <vector>

class InventoryTransactionPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::InventoryTransaction;

    InventoryTransactionPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "InventoryTransactionPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int32_t mLegacyRequestId = 0;
    std::vector<LegacySetItemSlotData> mLegacySlots;
    std::vector<InventoryActionData> mActions;
    InventoryTransactionType mTransactionType = InventoryTransactionType::Normal;
    int32_t mActionType = 0;
    int64_t mRuntimeActorId = 0;
    Vector3i mBlockPosition;
    int32_t mBlockFace = 0;
    int32_t mHotbarSlot = 0;
    ItemStack mItemInHand;
    Vector3f mPlayerPosition;
    Vector3f mClickPosition;
    Vector3f mHeadPosition;
    std::shared_ptr<BlockDefinition> mBlockDefinition;
    ItemUseTriggerType mTriggerType = ItemUseTriggerType::Unknown;
    ItemUsePredictedResult mClientInteractPrediction = ItemUsePredictedResult::Failure;
    int32_t mClientCooldownState = 0;
};
