#include "Inventory/ItemStackRequestHandler.h"

#include "Core/Debug/BedrockLog.h"

#include <algorithm>
#include <vector>

namespace {

    struct TouchedSlot {
        ContainerSlotType mContainer = ContainerSlotType::Unknown;
        int mSlot = 0;
    };

    struct RequestContext {
        const std::vector<CreativeItemData> *mCreativeItems = nullptr;
        const std::vector<ItemStack> *mRecipeOutputs = nullptr;
        ItemStack mCreatedOutput;
        std::vector<ItemStack> *mDroppedItems = nullptr;
    };

    ItemStack *resolveSlot(PlayerInventory &inventory, RequestContext &context, ContainerSlotType container,
                           int slot) {
        if (container == ContainerSlotType::CreatedOutput) {
            return &context.mCreatedOutput;
        }
        return inventory.resolveSlot(container, slot);
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
            return true;
        }

        return false;
    }

    bool craftRecipe(RequestContext &context, const ItemStackRequestAction &action) {
        if (context.mRecipeOutputs == nullptr) {
            return false;
        }

        const int32_t netId = action.mRecipeNetworkId;
        if (netId <= 0 || (size_t) netId > context.mRecipeOutputs->size()) {
            return false;
        }

        const ItemStack &output = (*context.mRecipeOutputs)[(size_t) netId - 1];
        if (output.isAir()) {
            return false;
        }

        const int repetitions = action.mNumberOfRequestedCrafts <= 0 ? 1 : action.mNumberOfRequestedCrafts;

        context.mCreatedOutput = output;
        context.mCreatedOutput.mCount = output.mCount * repetitions;
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
                return craftRecipe(context, action);

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
                                                       std::vector<ItemStack> *outDroppedItems) {
    ItemStackResponseEntry entry;
    entry.mRequestId = request.mRequestId;
    entry.mResult = RESULT_OK;

    std::vector<TouchedSlot> touched;

    RequestContext context;
    context.mCreativeItems = &creativeItems;
    context.mRecipeOutputs = &recipeOutputs;
    context.mCreatedOutput = ItemStack::air();
    context.mDroppedItems = outDroppedItems;

    for (const ItemStackRequestAction &action: request.mActions) {
        if (!applyAction(inventory, context, action, touched)) {
            entry.mResult = RESULT_ERROR;
            entry.mContainers.clear();
            return entry;
        }
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
