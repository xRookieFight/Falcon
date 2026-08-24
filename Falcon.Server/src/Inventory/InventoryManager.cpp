#include "Inventory/InventoryManager.h"

#include "Actor/ServerPlayer.h"
#include "Network/PacketSender.h"
#include "Protocol/Packets/ContainerClosePacket.h"
#include "Protocol/Packets/ContainerOpenPacket.h"
#include "Protocol/Packets/InventoryContentPacket.h"
#include "Protocol/Packets/InventorySlotPacket.h"
#include "Protocol/Packets/MobEquipmentPacket.h"
#include "Protocol/Types/ContainerSlotType.h"

#include <vector>

namespace {
    const ItemStack &itemAt(const PlayerInventory &inventory, InventoryManager::InventoryId id, int slot) {
        switch (id) {
            case InventoryManager::InventoryId::Inventory:
                return inventory.getItem(slot);
            case InventoryManager::InventoryId::Offhand:
                return inventory.getOffhand();
            case InventoryManager::InventoryId::Armor:
                return inventory.getArmor(slot);
            case InventoryManager::InventoryId::Cursor:
                return inventory.getCursor();
            case InventoryManager::InventoryId::CraftingInput:
                return inventory.getCraftingItem(slot);
        }
        return PlayerInventory::getEmptyItem();
    }
}

InventoryManager::InventoryManager()
        : mPlayer(nullptr), mSender(nullptr), mLastInventoryNetworkId(CONTAINER_ID_FIRST),
          mCurrentWindowType(ContainerType::Container), mMainInventoryWindowId(CONTAINER_ID_NONE),
          mClientSelectedHotbarSlot(-1), mHasPendingCloseWindow(false), mPendingCloseWindowId(CONTAINER_ID_NONE),
          mHasPendingOpenMainInventory(false) {}

void InventoryManager::attach(ServerPlayer *player, PacketSender *sender) {
    mPlayer = player;
    mSender = sender;
    mLastInventoryNetworkId = CONTAINER_ID_FIRST;
    mCurrentWindowType = ContainerType::Container;
    mMainInventoryWindowId = CONTAINER_ID_NONE;
    mClientSelectedHotbarSlot = -1;
    mHasPendingCloseWindow = false;
    mPendingCloseWindowId = CONTAINER_ID_NONE;
    mHasPendingOpenMainInventory = false;
}

int InventoryManager::_getNewWindowId() {
    int next = (mLastInventoryNetworkId + 1) % CONTAINER_ID_LAST;
    if (next < CONTAINER_ID_FIRST) {
        next = CONTAINER_ID_FIRST;
    }

    mLastInventoryNetworkId = next;
    return mLastInventoryNetworkId;
}

int InventoryManager::_getWindowId(InventoryId inventory) const {
    switch (inventory) {
        case InventoryId::Inventory:
            return CONTAINER_ID_INVENTORY;
        case InventoryId::Offhand:
            return CONTAINER_ID_OFFHAND;
        case InventoryId::Armor:
            return CONTAINER_ID_ARMOR;
        case InventoryId::Cursor:
            return CONTAINER_ID_UI;
        case InventoryId::CraftingInput:
            return CONTAINER_ID_UI;
    }
    return CONTAINER_ID_NONE;
}

int InventoryManager::_getSize(InventoryId inventory) const {
    switch (inventory) {
        case InventoryId::Inventory:
            return PlayerInventory::CONTAINER_SIZE;
        case InventoryId::Armor:
            return PlayerInventory::ARMOR_SIZE;
        case InventoryId::Offhand:
        case InventoryId::Cursor:
            return 1;
        case InventoryId::CraftingInput:
            return PlayerInventory::CRAFTING_SIZE;
    }
    return 0;
}

void InventoryManager::_sendContentPackets(int containerId, InventoryId inventory) {
    const PlayerInventory &contents = mPlayer->getInventory();
    const int size = _getSize(inventory);

    InventoryContentPacket clear;
    clear.mContainerId = containerId;
    clear.mContents.assign((size_t) size, ItemStack::air());
    clear.mContainerNameData.mContainer = ContainerSlotType::AnvilInput;
    clear.mStorageItem = ItemStack::air();
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), clear);

    InventoryContentPacket content;
    content.mContainerId = containerId;
    content.mContents.reserve((size_t) size);
    for (int slot = 0; slot < size; slot++) {
        content.mContents.push_back(itemAt(contents, inventory, slot));
    }
    content.mContainerNameData.mContainer = ContainerSlotType::AnvilInput;
    content.mStorageItem = ItemStack::air();
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), content);
}

void InventoryManager::_sendSlotPackets(int containerId, int netSlot, InventoryId inventory, int slot) {
    InventorySlotPacket clear;
    clear.mContainerId = containerId;
    clear.mSlot = netSlot;
    clear.mItem = ItemStack::air();
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), clear);

    InventorySlotPacket packet;
    packet.mContainerId = containerId;
    packet.mSlot = netSlot;
    packet.mItem = itemAt(mPlayer->getInventory(), inventory, slot);
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), packet);
}

void InventoryManager::syncContents(InventoryId inventory) {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    if (inventory == InventoryId::Cursor) {
        _sendSlotPackets(CONTAINER_ID_UI, UI_SLOT_CURSOR, inventory, 0);
        return;
    }

    if (inventory == InventoryId::CraftingInput) {
        for (int slot = 0; slot < PlayerInventory::CRAFTING_SIZE; slot++) {
            _sendSlotPackets(CONTAINER_ID_UI, PlayerInventory::CRAFTING_NETWORK_SLOT_FIRST + slot, inventory, slot);
        }
        return;
    }

    _sendContentPackets(_getWindowId(inventory), inventory);
}

void InventoryManager::syncSlot(InventoryId inventory, int slot) {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    if (inventory == InventoryId::Cursor) {
        _sendSlotPackets(CONTAINER_ID_UI, UI_SLOT_CURSOR, inventory, 0);
        return;
    }

    if (inventory == InventoryId::CraftingInput) {
        if (slot < 0 || slot >= PlayerInventory::CRAFTING_SIZE) {
            return;
        }
        _sendSlotPackets(CONTAINER_ID_UI, PlayerInventory::CRAFTING_NETWORK_SLOT_FIRST + slot, inventory, slot);
        return;
    }

    if (inventory == InventoryId::Offhand) {
        _sendContentPackets(CONTAINER_ID_OFFHAND, inventory);
        return;
    }

    _sendSlotPackets(_getWindowId(inventory), slot, inventory, slot);
}

void InventoryManager::syncAll() {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    syncContents(InventoryId::Inventory);
    syncContents(InventoryId::Offhand);
    syncContents(InventoryId::Armor);
    syncContents(InventoryId::CraftingInput);
    syncContents(InventoryId::Cursor);
}

void InventoryManager::onClientSelectHotbarSlot(int slot) {
    mClientSelectedHotbarSlot = slot;
}

void InventoryManager::syncSelectedHotbarSlot() {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    const PlayerInventory &inventory = mPlayer->getInventory();
    const int selected = inventory.getSelectedSlot();
    if (selected == mClientSelectedHotbarSlot) {
        return;
    }

    MobEquipmentPacket equipment;
    equipment.mRuntimeActorId = (int64_t) mPlayer->getRuntimeId();
    equipment.mItem = inventory.getItemInHand();
    equipment.mInventorySlot = selected;
    equipment.mHotbarSlot = selected;
    equipment.mContainerId = CONTAINER_ID_INVENTORY;
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), equipment);

    mClientSelectedHotbarSlot = selected;
}

void InventoryManager::onCurrentWindowRemove() {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    if (mMainInventoryWindowId == CONTAINER_ID_NONE) {
        return;
    }

    ContainerClosePacket close;
    close.mWindowId = (int8_t) mMainInventoryWindowId;
    close.mType = mCurrentWindowType;
    close.mServerInitiated = true;
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), close);

    mPendingCloseWindowId = mMainInventoryWindowId;
    mHasPendingCloseWindow = true;
    mMainInventoryWindowId = CONTAINER_ID_NONE;
}

void InventoryManager::onClientOpenMainInventory() {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    if (mMainInventoryWindowId != CONTAINER_ID_NONE) {
        return;
    }

    if (mHasPendingCloseWindow) {
        mHasPendingOpenMainInventory = true;
        return;
    }

    const int windowId = _getNewWindowId();
    mMainInventoryWindowId = windowId;
    mCurrentWindowType = ContainerType::Inventory;

    ContainerOpenPacket open;
    open.mWindowId = (int8_t) windowId;
    open.mType = mCurrentWindowType;
    open.mBlockPosition = Vector3i(0, 0, 0);
    open.mUniqueActorId = mPlayer->getUniqueId();
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), open);
}

void InventoryManager::onClientRemoveWindow(int windowId) {
    if (mPlayer == nullptr || mSender == nullptr) {
        return;
    }

    if (windowId == CONTAINER_ID_NONE) {
        windowId = mLastInventoryNetworkId;
    }

    if (windowId == mMainInventoryWindowId) {
        mMainInventoryWindowId = CONTAINER_ID_NONE;
    }

    ContainerClosePacket close;
    close.mWindowId = (int8_t) windowId;
    close.mType = mCurrentWindowType;
    close.mServerInitiated = false;
    mSender->sendPacketTo(mPlayer->getNetworkIdentifier(), close);

    if (mHasPendingCloseWindow && mPendingCloseWindowId == windowId) {
        mHasPendingCloseWindow = false;
        mPendingCloseWindowId = CONTAINER_ID_NONE;

        if (mHasPendingOpenMainInventory) {
            mHasPendingOpenMainInventory = false;
            onClientOpenMainInventory();
        }
    }
}
