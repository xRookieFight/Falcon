#include "Network/ItemEntityHandler.h"

#include "Entity/EntityFlags.h"
#include "Entity/ItemEntity.h"
#include "Entity/ServerPlayer.h"
#include "Inventory/InventoryManager.h"
#include "Inventory/PlayerInventory.h"
#include "Level/Level.h"
#include "Network/NetworkHandler.h"
#include "Network/ServerNetworkHandler.h"
#include "Protocol/Packets/AddItemEntityPacket.h"
#include "Protocol/Packets/MoveEntityAbsolutePacket.h"
#include "Protocol/Packets/RemoveEntityPacket.h"
#include "Protocol/Packets/TakeItemEntityPacket.h"
#include "Protocol/Types/StartGameTypes.h"

#include <cmath>

namespace {
    const float ITEM_GRAVITY = 0.04f;
    const float ITEM_HORIZONTAL_DRAG = 0.02f;
    const float ITEM_PICKUP_RANGE_SQUARED = 2.25f;

    int freeSpaceFor(const PlayerInventory &inventory, const ItemStack &item) {
        const int maxStackSize = PlayerInventory::getMaxStackSize(item);
        int free = 0;

        for (int slot = 0; slot < PlayerInventory::CONTAINER_SIZE; slot++) {
            const ItemStack &existing = inventory.getItem(slot);

            if (existing.isAir()) {
                free += maxStackSize;
                continue;
            }

            if (PlayerInventory::canStack(existing, item))
                free += maxStackSize - existing.mCount;
        }

        return free;
    }

    void spawnItemEntityTo(ServerNetworkHandler &owner, ServerPlayer &player, const ItemEntity &entity) {
        AddItemEntityPacket add;
        add.mUniqueEntityId = entity.getUniqueId();
        add.mRuntimeEntityId = entity.getRuntimeId();
        add.mItemInHand = entity.getItem();
        add.mPosition = entity.getPosition();
        add.mMotion = entity.getMotion();
        add.mFromFishing = false;

        EntityDataEntry flags;
        flags.mId = EntityFlags::FLAGS_DATA_ID;
        flags.mFormat = EntityDataFormat::Long;
        flags.mLongValue = entity.getFlags().getLowBits();
        add.mMetadata.mEntries.push_back(flags);

        EntityDataEntry flags2;
        flags2.mId = EntityFlags::FLAGS_2_DATA_ID;
        flags2.mFormat = EntityDataFormat::Long;
        flags2.mLongValue = entity.getFlags().getHighBits();
        add.mMetadata.mEntries.push_back(flags2);

        owner.getNetworkHandler().send(player.getNetworkIdentifier(), add, owner.getCodecContext());
    }

    void broadcastItemEntitySpawn(ServerNetworkHandler &owner, const ItemEntity &entity) {
        for (auto &entry: owner.getPlayers()) {
            if (entry.second.isSpawned())
                spawnItemEntityTo(owner, entry.second, entity);
        }
    }

    void broadcastItemEntityMove(ServerNetworkHandler &owner, const ItemEntity &entity) {
        MoveEntityAbsolutePacket move;
        move.mRuntimeEntityId = (int64_t) entity.getRuntimeId();
        move.mPosition = entity.getPosition();
        move.mRotation = entity.getRotation();
        move.mOnGround = entity.isOnGround();
        move.mTeleported = false;
        move.mForceMove = false;

        for (auto &entry: owner.getPlayers()) {
            if (entry.second.isSpawned())
                owner.getNetworkHandler().send(entry.second.getNetworkIdentifier(), move, owner.getCodecContext());
        }
    }

    void broadcastItemEntityRemove(ServerNetworkHandler &owner, const ItemEntity &entity) {
        RemoveEntityPacket remove;
        remove.mUniqueEntityId = entity.getUniqueId();

        for (auto &entry: owner.getPlayers()) {
            if (entry.second.isSpawned())
                owner.getNetworkHandler().send(entry.second.getNetworkIdentifier(), remove, owner.getCodecContext());
        }
    }

    void moveItemEntity(ServerNetworkHandler &owner, ItemEntity &entity) {
        if (entity.isOnGround())
            return;

        Vector3f motion = entity.getMotion();
        const Vector3f position = entity.getPosition();

        motion.y -= ITEM_GRAVITY;

        Vector3f next(position.x + motion.x, position.y + motion.y, position.z + motion.z);

        const int32_t blockX = (int32_t) std::floor(next.x);
        const int32_t blockZ = (int32_t) std::floor(next.z);
        const int32_t blockY = (int32_t) std::floor(next.y);

        if (motion.y < 0.0f && owner.getLevel().isSolidAt(blockX, blockY, blockZ)) {
            next.y = (float) (blockY + 1);
            motion = Vector3f(0.0f, 0.0f, 0.0f);
            entity.setOnGround(true);
        } else {
            motion.x *= 1.0f - ITEM_HORIZONTAL_DRAG;
            motion.z *= 1.0f - ITEM_HORIZONTAL_DRAG;
        }

        entity.setPosition(next);
        entity.setMotion(motion);

        broadcastItemEntityMove(owner, entity);
    }

    void tryPickupItemEntity(ServerNetworkHandler &owner, ItemEntity &entity) {
        if (entity.isRemoved() || !entity.canPickup())
            return;

        const Vector3f itemPosition = entity.getPosition();

        for (auto &entry: owner.getPlayers()) {
            ServerPlayer &player = entry.second;

            if (!player.isSpawned() || player.isDead())
                continue;

            if (player.getGameType() == (int32_t) GameType::Spectator)
                continue;

            const Vector3f playerPosition = player.getPosition();
            const float deltaX = playerPosition.x - itemPosition.x;
            const float deltaY = playerPosition.y + 1.0f - itemPosition.y;
            const float deltaZ = playerPosition.z - itemPosition.z;

            if (deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ > ITEM_PICKUP_RANGE_SQUARED)
                continue;

            PlayerInventory &inventory = player.getInventory();
            if (freeSpaceFor(inventory, entity.getItem()) < entity.getItem().mCount)
                continue;

            std::vector<int> touchedSlots;
            if (!inventory.addItem(entity.getItem(), touchedSlots))
                continue;

            TakeItemEntityPacket take;
            take.mItemRuntimeEntityId = entity.getRuntimeId();
            take.mRuntimeEntityId = player.getRuntimeId();

            for (auto &viewer: owner.getPlayers()) {
                if (viewer.second.isSpawned())
                    owner.getNetworkHandler().send(viewer.second.getNetworkIdentifier(), take, owner.getCodecContext());
            }

            for (int slot: touchedSlots)
                player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory, slot);

            entity.setRemoved(true);
            return;
        }
    }
}

ItemEntity *ItemEntityHandler::dropItem(ServerNetworkHandler &owner, const Vector3f &position, const ItemStack &item,
                                        const Vector3f &motion, int pickupDelay) {
    if (item.isAir() || item.mCount <= 0)
        return nullptr;

    std::unique_ptr<ItemEntity> entity(new ItemEntity(owner.allocateRuntimeId(), item));
    entity->getItem().mUsingNetId = false;
    entity->getItem().mNetId = 0;
    entity->setPosition(position);
    entity->setMotion(motion);
    entity->setPickupDelay(pickupDelay);
    entity->resetFallDistance();

    ItemEntity *result = entity.get();
    owner.getItemEntities().push_back(std::move(entity));

    broadcastItemEntitySpawn(owner, *result);
    return result;
}

void ItemEntityHandler::sendItemEntitiesTo(ServerNetworkHandler &owner, ServerPlayer &player) {
    for (const std::unique_ptr<ItemEntity> &entity: owner.getItemEntities()) {
        if (!entity->isRemoved())
            spawnItemEntityTo(owner, player, *entity);
    }
}

void ItemEntityHandler::tickItemEntities(ServerNetworkHandler &owner) {
    std::vector<std::unique_ptr<ItemEntity>> &entities = owner.getItemEntities();

    for (auto it = entities.begin(); it != entities.end();) {
        ItemEntity &entity = **it;

        entity.tick();
        moveItemEntity(owner, entity);
        tryPickupItemEntity(owner, entity);

        if (entity.isRemoved() || entity.isExpired()) {
            broadcastItemEntityRemove(owner, entity);
            it = entities.erase(it);
            continue;
        }

        ++it;
    }
}
