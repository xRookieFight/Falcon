#include "Network/InventoryHandler.h"

#include "Core/Debug/BedrockLog.h"
#include "Entity/ServerPlayer.h"
#include "Inventory/InventoryManager.h"
#include "Inventory/ItemStackRequestHandler.h"
#include "Inventory/PlayerInventory.h"
#include "Network/BadPacketCheck.h"
#include "Network/NetworkHandler.h"
#include "Network/ServerNetworkHandler.h"
#include "Protocol/Packets/ContainerClosePacket.h"
#include "Protocol/Packets/InventoryTransactionPacket.h"
#include "Protocol/Packets/ItemStackRequestPacket.h"
#include "Protocol/Packets/ItemStackResponsePacket.h"
#include "Protocol/Packets/MobArmorEquipmentPacket.h"
#include "Protocol/Packets/MobEquipmentPacket.h"
#include "Protocol/Packets/PlayerHotbarPacket.h"
#include "Protocol/Types/ContainerSlotType.h"
#include "Protocol/Types/InventoryActionData.h"
#include "Protocol/Types/InventorySource.h"

void InventoryHandler::sendInventory(ServerNetworkHandler &owner, ServerPlayer &player) {
    player.getInventoryManager().syncAll();
    player.getInventoryManager().syncSelectedHotbarSlot();
    sendArmorContent(owner, player);
    sendOffhandContent(owner, player);
    sendHeldItem(owner, player);
}

void InventoryHandler::sendArmorContent(ServerNetworkHandler &owner, ServerPlayer &player) {
    const PlayerInventory &inventory = player.getInventory();

    MobArmorEquipmentPacket equipment;
    equipment.mRuntimeEntityId = (int64_t) player.getRuntimeId();
    equipment.mHelmet = inventory.getArmor(PlayerInventory::ARMOR_HEAD);
    equipment.mChestplate = inventory.getArmor(PlayerInventory::ARMOR_TORSO);
    equipment.mLeggings = inventory.getArmor(PlayerInventory::ARMOR_LEGS);
    equipment.mBoots = inventory.getArmor(PlayerInventory::ARMOR_FEET);
    equipment.mBody = ItemStack::air();

    for (auto &entry: owner.getPlayers()) {
        if (entry.second.isSpawned() && &entry.second != &player)
            owner.getNetworkHandler().send(entry.second.getNetworkIdentifier(), equipment, owner.getCodecContext());
    }
}

void InventoryHandler::sendOffhandContent(ServerNetworkHandler &owner, ServerPlayer &player) {
    const ItemStack &offhand = player.getInventory().getOffhand();

    MobEquipmentPacket equipment;
    equipment.mRuntimeEntityId = (int64_t) player.getRuntimeId();
    equipment.mItem = offhand;
    equipment.mInventorySlot = PlayerInventory::OFFHAND_NETWORK_SLOT;
    equipment.mHotbarSlot = PlayerInventory::OFFHAND_NETWORK_SLOT;
    equipment.mContainerId = PlayerInventory::CONTAINER_ID_OFFHAND;

    for (auto &entry: owner.getPlayers()) {
        if (entry.second.isSpawned())
            owner.getNetworkHandler().send(entry.second.getNetworkIdentifier(), equipment, owner.getCodecContext());
    }
}

void InventoryHandler::sendHeldItem(ServerNetworkHandler &owner, ServerPlayer &player) {
    const PlayerInventory &inventory = player.getInventory();
    const int selected = inventory.getSelectedSlot();

    player.getInventoryManager().syncSelectedHotbarSlot();

    MobEquipmentPacket equipment;
    equipment.mRuntimeEntityId = (int64_t) player.getRuntimeId();
    equipment.mItem = inventory.getItemInHand();
    equipment.mInventorySlot = selected;
    equipment.mHotbarSlot = selected;
    equipment.mContainerId = PlayerInventory::CONTAINER_ID_INVENTORY;

    for (auto &entry: owner.getPlayers()) {
        if (entry.second.isSpawned() && &entry.second != &player)
            owner.getNetworkHandler().send(entry.second.getNetworkIdentifier(), equipment, owner.getCodecContext());
    }
}

void InventoryHandler::handleMobEquipment(ServerNetworkHandler &owner, ServerPlayer &player,
                                          const MobEquipmentPacket &packet) {
    if (packet.mContainerId != PlayerInventory::CONTAINER_ID_INVENTORY)
        return;

    PlayerInventory &inventory = player.getInventory();

    player.getInventoryManager().onClientSelectHotbarSlot(packet.mHotbarSlot);
    inventory.setSelectedSlot(packet.mHotbarSlot);

    MobEquipmentPacket equipment;
    equipment.mRuntimeEntityId = (int64_t) player.getRuntimeId();
    equipment.mItem = inventory.getItemInHand();
    equipment.mInventorySlot = inventory.getSelectedSlot();
    equipment.mHotbarSlot = inventory.getSelectedSlot();
    equipment.mContainerId = PlayerInventory::CONTAINER_ID_INVENTORY;

    for (auto &entry: owner.getPlayers()) {
        if (entry.second.isSpawned() && &entry.second != &player)
            owner.getNetworkHandler().send(entry.second.getNetworkIdentifier(), equipment, owner.getCodecContext());
    }
}

void InventoryHandler::handlePlayerHotbar(ServerNetworkHandler &owner, ServerPlayer &player,
                                          const PlayerHotbarPacket &packet) {
    if (packet.mContainerId != PlayerInventory::CONTAINER_ID_INVENTORY || !packet.mSelectHotbarSlot)
        return;

    if (packet.mSelectedHotbarSlot < 0 || packet.mSelectedHotbarSlot >= PlayerInventory::HOTBAR_SIZE) {
        sendHeldItem(owner, player);
        return;
    }

    player.getInventoryManager().onClientSelectHotbarSlot(packet.mSelectedHotbarSlot);
    player.getInventory().setSelectedSlot(packet.mSelectedHotbarSlot);
}

void InventoryHandler::handleOpenInventory(ServerPlayer &player) {
    player.getInventoryManager().onClientOpenMainInventory();
}

void InventoryHandler::handleItemStackRequest(ServerNetworkHandler &owner, const NetworkIdentifier &id,
                                              ServerPlayer &player, const ItemStackRequestPacket &packet) {
    PlayerInventory &inventory = player.getInventory();

    ItemStackResponsePacket response;
    bool needsResync = false;

    std::vector<ItemStack> droppedItems;

    for (const ItemStackRequest &request: packet.mRequests) {
        ItemStackResponseEntry entry = ItemStackRequestHandler::execute(inventory, request, owner.getCreativeItems(),
                                                                       owner.getRecipeOutputs(), &droppedItems);
        if (entry.mResult != ItemStackRequestHandler::RESULT_OK)
            needsResync = true;

        response.mEntries.push_back(std::move(entry));
    }

    for (const ItemStack &dropped: droppedItems)
        owner._throwItem(player, dropped);

    if (response.mEntries.empty())
        return;

    owner.getNetworkHandler().send(id, response, owner.getCodecContext());

    if (needsResync)
        owner._sendInventory(player);
}

void InventoryHandler::handleTransaction(ServerNetworkHandler &owner, ServerPlayer &player,
                                         const InventoryTransactionPacket &packet) {
    if (packet.mTransactionType == InventoryTransactionType::ItemUse) {
        if (packet.mActionType == 1)
            owner._useHeldItem(player);
        return;
    }

    if (packet.mTransactionType == InventoryTransactionType::ItemRelease) {
        owner._consumeHeldItem(player);
        return;
    }

    if (packet.mTransactionType != InventoryTransactionType::Normal
        && packet.mTransactionType != InventoryTransactionType::Mismatch)
        return;

    PlayerInventory &inventory = player.getInventory();

    int32_t sourceSlot = -1;
    int32_t droppedCount = -1;

    for (const InventoryActionData &action: packet.mActions) {
        if (action.mSource.mType == InventorySourceType::WorldInteraction && action.mSlot == 0) {
            droppedCount = action.mToItem.mCount;
            continue;
        }

        if (action.mSource.mType == InventorySourceType::Container
            && action.mSource.mContainerId == PlayerInventory::CONTAINER_ID_INVENTORY) {
            sourceSlot = action.mSlot;
        }
    }

    if (sourceSlot < 0 || droppedCount <= 0)
        return;

    ItemStack *item = inventory.resolveSlot(ContainerSlotType::Inventory, sourceSlot);
    if (item == nullptr || item->isAir() || droppedCount > item->mCount)
        return;

    ItemStack thrown = *item;
    thrown.mCount = droppedCount;
    thrown.mUsingNetId = false;
    thrown.mNetId = 0;

    item->mCount -= droppedCount;
    if (item->mCount <= 0)
        *item = ItemStack::air();

    inventory.assignNetId(*item);
    owner._throwItem(player, thrown);

    player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory, sourceSlot);
}

void InventoryHandler::handleContainerClose(ServerPlayer &player, const ContainerClosePacket &packet) {
    PlayerInventory &inventory = player.getInventory();
    if (!inventory.getCursor().isAir()) {
        std::vector<int> touchedSlots;
        inventory.addItem(inventory.getCursor(), touchedSlots);
        inventory.setCursor(ItemStack::air());

        for (int slot: touchedSlots)
            player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory, slot);
    }

    player.getInventoryManager().onClientRemoveWindow((int) packet.mWindowId);
}
