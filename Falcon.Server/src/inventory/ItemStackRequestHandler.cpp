#include "inventory/ItemStackRequestHandler.h"

#include "core/debug/BedrockLog.h"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace {

    struct TouchedSlot {
        ContainerSlotType mContainer = ContainerSlotType::Unknown;
        int mSlot = 0;
    };

    struct RequestContext {
        const std::vector<CreativeItemData> *mCreativeItems = nullptr;
        const std::vector<ItemStack> *mRecipeOutputs = nullptr;
        const std::vector<uint32_t> *mRecipeSourceIndices = nullptr;
        ItemStack mCreatedOutput;
        std::vector<ItemStack> *mDroppedItems = nullptr;
        bool mCraftingTableOpen = false;
        bool mFurnaceOpen = false;
        CraftingRecipeMatch mCraftingMatch;
        std::vector<int32_t> mCraftingRequired;
        std::vector<int32_t> mCraftingConsumed;
        int mCraftingGridWidth = 0;
        bool mCraftingActive = false;
        bool mCraftRecipeSeen = false;
        bool mCreatedOutputActive = false;
    };

    ItemStack *resolveSlot(PlayerInventory &inventory, RequestContext &context, ContainerSlotType container,
                           int slot) {
        if (container == ContainerSlotType::CreatedOutput) {
            return (slot == 0 || slot == 50) ? &context.mCreatedOutput : nullptr;
        }
        if (container == ContainerSlotType::CraftingInput
            && slot >= PlayerInventory::CRAFTING_TABLE_NETWORK_SLOT_FIRST
            && slot < PlayerInventory::CRAFTING_TABLE_NETWORK_SLOT_FIRST + PlayerInventory::CRAFTING_TABLE_SIZE
            && !context.mCraftingTableOpen) {
            return nullptr;
        }
        if (container == ContainerSlotType::FurnaceIngredient || container == ContainerSlotType::BlastFurnaceIngredient
            || container == ContainerSlotType::SmokerIngredient || container == ContainerSlotType::FurnaceFuel
            || container == ContainerSlotType::FurnaceResult) {
            if (!context.mFurnaceOpen) {
                return nullptr;
            }
            return inventory.resolveSlot(container, slot);
        }
        return inventory.resolveSlot(container, slot);
    }

    int craftingSlotIndex(ContainerSlotType container, int slot, int gridWidth) {
        if (container != ContainerSlotType::CraftingInput) {
            return -1;
        }
        if (gridWidth == 2 && slot >= PlayerInventory::CRAFTING_NETWORK_SLOT_FIRST
            && slot < PlayerInventory::CRAFTING_NETWORK_SLOT_FIRST + PlayerInventory::CRAFTING_SIZE) {
            return slot - PlayerInventory::CRAFTING_NETWORK_SLOT_FIRST;
        }
        if (gridWidth == 3 && slot >= PlayerInventory::CRAFTING_TABLE_NETWORK_SLOT_FIRST
            && slot < PlayerInventory::CRAFTING_TABLE_NETWORK_SLOT_FIRST + PlayerInventory::CRAFTING_TABLE_SIZE) {
            return slot - PlayerInventory::CRAFTING_TABLE_NETWORK_SLOT_FIRST;
        }
        return -1;
    }

    void markTouched(std::vector<TouchedSlot> &touched, ContainerSlotType container, int slot) {
        for (const TouchedSlot &entry: touched) {
            if (entry.mContainer == container && entry.mSlot == slot) {
                return;
            }
        }

        TouchedSlot entry;
        entry.mContainer = container;
        entry.mSlot = slot;
        touched.push_back(entry);
    }

    ItemStackResponseSlot describeSlot(int slot, const ItemStack &item) {
        ItemStackResponseSlot response;
        response.mSlot = slot;
        response.mHotbarSlot = slot;
        response.mCount = item.isAir() ? 0 : item.mCount;
        response.mStackNetworkId = item.mUsingNetId ? item.mNetId : 0;
        response.mDurabilityCorrection = item.isAir() ? 0 : item.mDamage;
        return response;
    }

    void appendResponseSlot(std::vector<ItemStackResponseContainer> &containers, ContainerSlotType container,
                            int slot, const ItemStack &item) {
        for (ItemStackResponseContainer &existing: containers) {
            if (existing.mContainerName.mContainer != container) {
                continue;
            }

            for (ItemStackResponseSlot &existingSlot: existing.mItems) {
                if (existingSlot.mSlot == slot) {
                    existingSlot = describeSlot(slot, item);
                    return;
                }
            }

            existing.mItems.push_back(describeSlot(slot, item));
            return;
        }

        ItemStackResponseContainer created;
        created.mContainerName.mContainer = container;
        created.mItems.push_back(describeSlot(slot, item));
        containers.push_back(created);
    }

    bool moveItems(PlayerInventory &inventory, RequestContext &context, const ItemStackRequestAction &action,
                   std::vector<TouchedSlot> &touched) {
        ItemStack *source = resolveSlot(inventory, context, action.mSource.mContainerName.mContainer,
                                        action.mSource.mSlot);
        ItemStack *destination = resolveSlot(inventory, context, action.mDestination.mContainerName.mContainer,
                                             action.mDestination.mSlot);

        if (source == nullptr || destination == nullptr || source == destination) {
            return false;
        }

        if (action.mDestination.mContainerName.mContainer == ContainerSlotType::FurnaceResult) {
            return false;
        }

        if (source->isAir() || action.mCount <= 0 || action.mCount > source->mCount) {
            return false;
        }

        if (!destination->isAir() && !PlayerInventory::canStack(*source, *destination)) {
            return false;
        }

        if (!destination->isAir()
            && destination->mCount + action.mCount > PlayerInventory::getMaxStackSize(*destination)) {
            return false;
        }

        if (destination->isAir()) {
            ItemStack moved = *source;
            moved.mCount = action.mCount;
            *destination = moved;
        } else {
            destination->mCount += action.mCount;
        }

        source->mCount -= action.mCount;
        if (source->mCount <= 0) {
            *source = ItemStack::air();
        }

        inventory.assignNetId(*source);
        inventory.assignNetId(*destination);

        markTouched(touched, action.mSource.mContainerName.mContainer, action.mSource.mSlot);
        markTouched(touched, action.mDestination.mContainerName.mContainer, action.mDestination.mSlot);
        return true;
    }

    bool swapItems(PlayerInventory &inventory, RequestContext &context, const ItemStackRequestAction &action,
                   std::vector<TouchedSlot> &touched) {
        ItemStack *source = resolveSlot(inventory, context, action.mSource.mContainerName.mContainer,
                                        action.mSource.mSlot);
        ItemStack *destination = resolveSlot(inventory, context, action.mDestination.mContainerName.mContainer,
                                             action.mDestination.mSlot);

        if (source == nullptr || destination == nullptr || source == destination) {
            return false;
        }

        if (action.mSource.mContainerName.mContainer == ContainerSlotType::FurnaceResult
            || action.mDestination.mContainerName.mContainer == ContainerSlotType::FurnaceResult) {
            return false;
        }

        std::swap(*source, *destination);

        inventory.assignNetId(*source);
        inventory.assignNetId(*destination);

        markTouched(touched, action.mSource.mContainerName.mContainer, action.mSource.mSlot);
        markTouched(touched, action.mDestination.mContainerName.mContainer, action.mDestination.mSlot);
        return true;
    }

    bool removeItems(PlayerInventory &inventory, RequestContext &context, const ItemStackRequestAction &action,
                     std::vector<TouchedSlot> &touched, bool dropped = false) {
        ItemStack *source = resolveSlot(inventory, context, action.mSource.mContainerName.mContainer,
                                        action.mSource.mSlot);
        if (source == nullptr || source->isAir() || action.mCount <= 0 || action.mCount > source->mCount) {
            return false;
        }

        if (action.mSource.mContainerName.mContainer == ContainerSlotType::FurnaceResult) {
            return false;
        }

        if (context.mCraftingActive && action.mType == ItemStackRequestActionType::Consume) {
            const int gridSlot = craftingSlotIndex(action.mSource.mContainerName.mContainer,
                                                   action.mSource.mSlot, context.mCraftingGridWidth);
            if (gridSlot < 0 || (size_t) gridSlot >= context.mCraftingRequired.size()
                || context.mCraftingConsumed[(size_t) gridSlot] + action.mCount
                   > context.mCraftingRequired[(size_t) gridSlot]) {
                return false;
            }
            context.mCraftingConsumed[(size_t) gridSlot] += action.mCount;
        }

        if (dropped && context.mDroppedItems != nullptr) {
            ItemStack drop = *source;
            drop.mCount = action.mCount;
            drop.mUsingNetId = false;
            drop.mNetId = 0;
            context.mDroppedItems->push_back(drop);
        }

        source->mCount -= action.mCount;
        if (source->mCount <= 0) {
            *source = ItemStack::air();
        }

        inventory.assignNetId(*source);
        markTouched(touched, action.mSource.mContainerName.mContainer, action.mSource.mSlot);
        return true;
    }

    bool createCreativeItem(RequestContext &context, const ItemStackRequestAction &action) {
        if (context.mCreativeItems == nullptr) {
            return false;
        }

        for (const CreativeItemData &entry: *context.mCreativeItems) {
            if (entry.mNetId != action.mCreativeItemNetworkId) {
                continue;
            }

            const int repetitions = action.mNumberOfRequestedCrafts <= 0 ? 1 : action.mNumberOfRequestedCrafts;

            context.mCreatedOutput = entry.mItem;
            context.mCreatedOutput.mCount = PlayerInventory::getMaxStackSize(entry.mItem) * repetitions;
            context.mCreatedOutputActive = true;
            return true;
        }

        return false;
    }

    bool craftRecipe(PlayerInventory &inventory, RequestContext &context, const ItemStackRequestAction &action) {
        if (context.mCraftRecipeSeen) {
            return false;
        }
        context.mCraftRecipeSeen = true;

        if (context.mRecipeOutputs == nullptr) {
            return false;
        }

        const int32_t netId = action.mRecipeNetworkId;
        if (context.mRecipeSourceIndices == nullptr || netId <= 0
            || (size_t) netId > context.mRecipeSourceIndices->size()) {
            return false;
        }

        const ItemStack &output = (*context.mRecipeOutputs)[(size_t) netId - 1];
        if (output.isAir()) {
            return false;
        }

        const int repetitions = action.mNumberOfRequestedCrafts <= 0 ? 1 : action.mNumberOfRequestedCrafts;
        if (repetitions > 256) {
            return false;
        }

        bool tableHasItems = false;
        for (const ItemStack &item: inventory.getCraftingTableContents()) {
            if (!item.isAir()) {
                tableHasItems = true;
                break;
            }
        }

        if (tableHasItems && !context.mCraftingTableOpen) {
            return false;
        }

        const int gridWidth = tableHasItems ? 3 : 2;
        const std::vector<ItemStack> &grid = tableHasItems
                                             ? inventory.getCraftingTableContents()
                                             : inventory.getCraftingContents();
        CraftingRecipeMatch match;
        if (!CraftingManager::matchNetworkRecipe(grid, gridWidth, netId, *context.mRecipeOutputs,
                                                 *context.mRecipeSourceIndices, match)) {
            return false;
        }

        std::vector<int32_t> required = match.mRequiredBySlot;
        for (int32_t &count: required) {
            count *= repetitions;
        }
        for (size_t slot = 0; slot < required.size(); ++slot) {
            if (required[slot] > 0 && grid[slot].mCount < required[slot]) {
                return false;
            }
        }

        context.mCreatedOutput = output;
        context.mCreatedOutput.mCount = output.mCount * repetitions;
        context.mCreatedOutputActive = true;
        context.mCraftingMatch = std::move(match);
        context.mCraftingRequired = std::move(required);
        context.mCraftingConsumed.assign(context.mCraftingRequired.size(), 0);
        context.mCraftingGridWidth = gridWidth;
        context.mCraftingActive = true;
        return true;
    }

    bool applyAction(PlayerInventory &inventory, RequestContext &context, const ItemStackRequestAction &action,
                     std::vector<TouchedSlot> &touched) {
        switch (action.mType) {
            case ItemStackRequestActionType::Take:
            case ItemStackRequestActionType::Place:
                return moveItems(inventory, context, action, touched);

            case ItemStackRequestActionType::Swap:
                return swapItems(inventory, context, action, touched);

            case ItemStackRequestActionType::Drop:
                return removeItems(inventory, context, action, touched, true);

            case ItemStackRequestActionType::Destroy:
            case ItemStackRequestActionType::Consume:
                return removeItems(inventory, context, action, touched);

            case ItemStackRequestActionType::CraftCreative:
                return createCreativeItem(context, action);

            case ItemStackRequestActionType::CraftRecipe:
            case ItemStackRequestActionType::CraftRecipeAuto:
                return craftRecipe(inventory, context, action);

            case ItemStackRequestActionType::Create:
            case ItemStackRequestActionType::CraftResultsDeprecated:
            case ItemStackRequestActionType::CraftNonImplemented:
            case ItemStackRequestActionType::MineBlock:
            case ItemStackRequestActionType::LabTableCombine:
            case ItemStackRequestActionType::BeaconPayment:
                return true;

            default:
                LOG_WARN(LogAreaID::Server, "Unhandled item stack request action %d",
                         itemStackRequestActionTypeToId(action.mType));
                return false;
        }
    }

}

ItemStackResponseEntry ItemStackRequestHandler::execute(PlayerInventory &inventory, const ItemStackRequest &request,
                                                       const std::vector<CreativeItemData> &creativeItems,
                                                       const std::vector<ItemStack> &recipeOutputs,
                                                       const std::vector<uint32_t> &recipeSourceIndices,
                                                       bool craftingTableOpen,
                                                       bool furnaceOpen,
                                                       std::vector<ItemStack> *outDroppedItems) {
    ItemStackResponseEntry entry;
    entry.mRequestId = request.mRequestId;
    entry.mResult = RESULT_OK;

    PlayerInventory working = inventory;
    std::vector<TouchedSlot> touched;
    std::vector<ItemStack> localDroppedItems;

    RequestContext context;
    context.mCreativeItems = &creativeItems;
    context.mRecipeOutputs = &recipeOutputs;
    context.mRecipeSourceIndices = &recipeSourceIndices;
    context.mCreatedOutput = ItemStack::air();
    context.mDroppedItems = &localDroppedItems;
    context.mCraftingTableOpen = craftingTableOpen;
    context.mFurnaceOpen = furnaceOpen;

    for (const ItemStackRequestAction &action: request.mActions) {
        if (!applyAction(working, context, action, touched)) {
            entry.mResult = RESULT_ERROR;
            entry.mContainers.clear();
            return entry;
        }
    }

    if (context.mCraftingActive) {
        if (context.mCraftingRequired.size() != context.mCraftingConsumed.size()) {
            entry.mResult = RESULT_ERROR;
            return entry;
        }
        for (size_t slot = 0; slot < context.mCraftingRequired.size(); ++slot) {
            if (context.mCraftingRequired[slot] != context.mCraftingConsumed[slot]) {
                entry.mResult = RESULT_ERROR;
                entry.mContainers.clear();
                return entry;
            }
        }
    }

    if (context.mCreatedOutputActive && !context.mCreatedOutput.isAir()) {
        entry.mResult = RESULT_ERROR;
        entry.mContainers.clear();
        return entry;
    }

    inventory = std::move(working);
    if (outDroppedItems != nullptr) {
        outDroppedItems->insert(outDroppedItems->end(), localDroppedItems.begin(), localDroppedItems.end());
    }

    for (const TouchedSlot &slot: touched) {
        if (slot.mContainer == ContainerSlotType::CreatedOutput) {
            continue;
        }

        const ItemStack *item = inventory.resolveSlot(slot.mContainer, slot.mSlot);
        if (item == nullptr) {
            continue;
        }

        appendResponseSlot(entry.mContainers, slot.mContainer, slot.mSlot, *item);
    }

    return entry;
}
