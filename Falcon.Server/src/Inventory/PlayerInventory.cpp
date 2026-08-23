#include "Inventory/PlayerInventory.h"

#include "Item/ItemData.h"

#include <utility>

namespace {
    const int DEFAULT_MAX_STACK_SIZE = 64;
}

PlayerInventory::PlayerInventory()
        : mItems((size_t) CONTAINER_SIZE), mArmor((size_t) ARMOR_SIZE), mCrafting((size_t) CRAFTING_SIZE),
          mSelectedSlot(0), mNextNetId(1) {
    mOffhand = ItemStack::air();
    mCursor = ItemStack::air();

    for (ItemStack &item: mItems) {
        item = ItemStack::air();
    }

    for (ItemStack &item: mArmor) {
        item = ItemStack::air();
    }

    for (ItemStack &item: mCrafting) {
        item = ItemStack::air();
    }
}

const ItemStack &PlayerInventory::getEmptyItem() {
    static const ItemStack empty = ItemStack::air();
    return empty;
}

const ItemStack &PlayerInventory::getItem(int slot) const {
    if (slot < 0 || slot >= CONTAINER_SIZE) {
        return getEmptyItem();
    }
    return mItems[(size_t) slot];
}

void PlayerInventory::setItem(int slot, ItemStack item) {
    if (slot < 0 || slot >= CONTAINER_SIZE) {
        return;
    }

    assignNetId(item);
    mItems[(size_t) slot] = std::move(item);
}

const ItemStack &PlayerInventory::getArmor(int slot) const {
    if (slot < 0 || slot >= ARMOR_SIZE) {
        return getEmptyItem();
    }
    return mArmor[(size_t) slot];
}

void PlayerInventory::setArmor(int slot, ItemStack item) {
    if (slot < 0 || slot >= ARMOR_SIZE) {
        return;
    }

    assignNetId(item);
    mArmor[(size_t) slot] = std::move(item);
}

const ItemStack &PlayerInventory::getCraftingItem(int slot) const {
    if (slot < 0 || slot >= CRAFTING_SIZE) {
        return getEmptyItem();
    }
    return mCrafting[(size_t) slot];
}

void PlayerInventory::setCraftingItem(int slot, ItemStack item) {
    if (slot < 0 || slot >= CRAFTING_SIZE) {
        return;
    }

    assignNetId(item);
    mCrafting[(size_t) slot] = std::move(item);
}

void PlayerInventory::setOffhand(ItemStack item) {
    assignNetId(item);
    mOffhand = std::move(item);
}

void PlayerInventory::setCursor(ItemStack item) {
    assignNetId(item);
    mCursor = std::move(item);
}

void PlayerInventory::setSelectedSlot(int slot) {
    if (slot < 0) {
        mSelectedSlot = 0;
        return;
    }

    if (slot >= HOTBAR_SIZE) {
        mSelectedSlot = HOTBAR_SIZE - 1;
        return;
    }

    mSelectedSlot = slot;
}

ItemStack *PlayerInventory::resolveSlot(ContainerSlotType container, int slot) {
    const PlayerInventory *self = this;
    return const_cast<ItemStack *>(self->resolveSlot(container, slot));
}

const ItemStack *PlayerInventory::resolveSlot(ContainerSlotType container, int slot) const {
    switch (container) {
        case ContainerSlotType::Hotbar:
        case ContainerSlotType::Inventory:
        case ContainerSlotType::HotbarAndInventory:
            if (slot < 0 || slot >= CONTAINER_SIZE) {
                return nullptr;
            }
            return &mItems[(size_t) slot];

        case ContainerSlotType::Armor:
            if (slot < 0 || slot >= ARMOR_SIZE) {
                return nullptr;
            }
            return &mArmor[(size_t) slot];

        case ContainerSlotType::Offhand:
            if (slot != 0 && slot != OFFHAND_NETWORK_SLOT) {
                return nullptr;
            }
            return &mOffhand;

        case ContainerSlotType::Cursor:
            if (slot != 0) {
                return nullptr;
            }
            return &mCursor;

        case ContainerSlotType::CraftingInput:
            if (slot < CRAFTING_NETWORK_SLOT_FIRST
                || slot >= CRAFTING_NETWORK_SLOT_FIRST + CRAFTING_SIZE) {
                return nullptr;
            }
            return &mCrafting[(size_t) (slot - CRAFTING_NETWORK_SLOT_FIRST)];

        default:
            return nullptr;
    }
}

int PlayerInventory::getFirstEmptySlot() const {
    for (int slot = 0; slot < CONTAINER_SIZE; slot++) {
        if (mItems[(size_t) slot].isAir()) {
            return slot;
        }
    }
    return -1;
}

bool PlayerInventory::canStack(const ItemStack &left, const ItemStack &right) {
    if (left.isAir() || right.isAir()) {
        return false;
    }

    if (left.mDefinition->getIdentifier() != right.mDefinition->getIdentifier()) {
        return false;
    }

    if (left.mDamage != right.mDamage) {
        return false;
    }

    return left.mTag == right.mTag;
}

int PlayerInventory::getMaxStackSize(const ItemStack &item) {
    if (item.isAir())
        return DEFAULT_MAX_STACK_SIZE;

    const ItemData *data = ItemDataTable::find(item.mDefinition->getIdentifier());
    if (data == nullptr)
        return DEFAULT_MAX_STACK_SIZE;

    return data->mMaxStackSize > 0 ? data->mMaxStackSize : DEFAULT_MAX_STACK_SIZE;
}

bool PlayerInventory::addItem(const ItemStack &item) {
    std::vector<int> touchedSlots;
    return addItem(item, touchedSlots);
}

bool PlayerInventory::addItem(const ItemStack &item, std::vector<int> &outTouchedSlots) {
    return addItemPartial(item, outTouchedSlots) == 0;
}

int PlayerInventory::addItemPartial(const ItemStack &item, std::vector<int> &outTouchedSlots) {
    if (item.isAir() || item.mCount <= 0) {
        return 0;
    }

    int remaining = item.mCount;
    const int maxStackSize = getMaxStackSize(item);

    for (int slot = 0; slot < CONTAINER_SIZE && remaining > 0; slot++) {
        ItemStack &existing = mItems[(size_t) slot];
        if (!canStack(existing, item)) {
            continue;
        }

        const int free = maxStackSize - existing.mCount;
        if (free <= 0) {
            continue;
        }

        const int moved = free < remaining ? free : remaining;
        existing.mCount += moved;
        remaining -= moved;
        assignNetId(existing);
        outTouchedSlots.push_back(slot);
    }

    while (remaining > 0) {
        const int slot = getFirstEmptySlot();
        if (slot < 0) {
            break;
        }

        ItemStack copy = item;
        copy.mCount = maxStackSize < remaining ? maxStackSize : remaining;
        remaining -= copy.mCount;
        setItem(slot, std::move(copy));
        outTouchedSlots.push_back(slot);
    }

    return remaining;
}

void PlayerInventory::clear() {
    for (ItemStack &item: mItems) {
        item = ItemStack::air();
    }

    for (ItemStack &item: mArmor) {
        item = ItemStack::air();
    }

    mOffhand = ItemStack::air();
    mCursor = ItemStack::air();

    for (ItemStack &item: mCrafting) {
        item = ItemStack::air();
    }
}

void PlayerInventory::assignNetId(ItemStack &item) {
    if (item.isAir() || item.mCount <= 0) {
        item = ItemStack::air();
        return;
    }

    item.mUsingNetId = true;
    item.mNetId = mNextNetId++;

    if (mNextNetId <= 0) {
        mNextNetId = 1;
    }
}
