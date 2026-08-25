#pragma once

#include "core/math/Vector3f.h"
#include "core/math/Vector3i.h"
#include "protocol/types/BlockDefinition.h"
#include "protocol/types/InventoryActionData.h"
#include "protocol/types/ItemStack.h"

#include <cstdint>
#include <memory>
#include <vector>

enum class ItemUsePredictedResult {
    Failure,
    Success,
};

enum class ItemUseTriggerType {
    Unknown,
    PlayerInput,
    SimulationTick,
};

class ItemUseTransaction {
public:
    int32_t mLegacyRequestId = 0;
    std::vector<InventoryActionData> mActions;
    int32_t mActionType = 0;
    Vector3i mBlockPosition;
    int32_t mBlockFace = 0;
    int32_t mHotbarSlot = 0;
    ItemStack mItemInHand;
    Vector3f mPlayerPosition;
    Vector3f mClickPosition;
    std::shared_ptr<BlockDefinition> mBlockDefinition;
    ItemUsePredictedResult mClientInteractPrediction = ItemUsePredictedResult::Failure;
    ItemUseTriggerType mTriggerType = ItemUseTriggerType::Unknown;
    int32_t mClientCooldownState = 0;
};
