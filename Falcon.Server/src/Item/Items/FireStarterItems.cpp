#include "Item/Items/FireStarterItems.h"

#include "Block/BlockData.h"
#include "Block/Systems/FireSystem.h"
#include "Inventory/InventoryManager.h"
#include "Inventory/PlayerInventory.h"
#include "Level/Level.h"
#include "Network/Handler/ServerNetworkHandler.h"
#include "Protocol/Packets/LevelSoundEventPacket.h"
#include "Protocol/Types/StartGameTypes.h"

namespace {
    Vector3i relativeToFace(const Vector3i &position, int32_t face) {
        switch (face) {
            case 0:
                return Vector3i(position.x, position.y - 1, position.z);
            case 1:
                return Vector3i(position.x, position.y + 1, position.z);
            case 2:
                return Vector3i(position.x, position.y, position.z - 1);
            case 3:
                return Vector3i(position.x, position.y, position.z + 1);
            case 4:
                return Vector3i(position.x - 1, position.y, position.z);
            default:
                return Vector3i(position.x + 1, position.y, position.z);
        }
    }

    Vector3f centerOf(const Vector3i &position) {
        return Vector3f((float) position.x + 0.5f, (float) position.y + 0.5f, (float) position.z + 0.5f);
    }

    bool isSolid(const std::string &identifier) {
        const BlockData *data = BlockDataTable::find(identifier.c_str());
        return data != nullptr && data->mSolid;
    }

    bool canIgniteAgainst(ServerNetworkHandler &owner, const Vector3i &target, const Vector3i &placement) {
        Level &level = owner.getLevel();

        if (level.getBlockState(placement.x, placement.y, placement.z).mName != "minecraft:air")
            return false;

        const std::string identifier = level.getBlockState(target.x, target.y, target.z).mName;
        const int burnChance = FireSystem::getBurnChance(identifier);

        return burnChance != FireSystem::UNBURNABLE && (isSolid(identifier) || burnChance > 0);
    }

    void consumeHeldOne(ServerPlayer &player) {
        if (player.getGameType() == (int32_t) GameType::Creative)
            return;

        PlayerInventory &inventory = player.getInventory();
        ItemStack held = inventory.getItemInHand();
        if (held.isAir())
            return;

        held.mCount -= 1;
        if (held.mCount <= 0)
            inventory.setItemInHand(ItemStack::air());
        else
            inventory.setItemInHand(std::move(held));

        player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory,
                                              inventory.getSelectedSlot());
    }
}

FlintAndSteelItem::FlintAndSteelItem(const Item &base) : Item(base) {}

bool FlintAndSteelItem::onUseOnBlock(ServerNetworkHandler &owner, ServerPlayer &player, const ItemStack &item,
                                     const Vector3i &blockPosition, int32_t face,
                                     const Vector3f &clickPosition) const {
    (void) item;
    (void) clickPosition;

    if (player.getGameType() == (int32_t) GameType::Adventure)
        return false;

    const Vector3i placement = relativeToFace(blockPosition, face);
    const bool ignitable = canIgniteAgainst(owner, blockPosition, placement);

    if (ignitable)
        FireSystem::ignite(owner, placement);

    owner.damagePlayerHeldItem(player, 1);
    owner.playLevelSound(LevelSoundEvent::FIRE_IGNITE, centerOf(placement));
    return ignitable;
}

FireChargeItem::FireChargeItem(const Item &base) : Item(base) {}

bool FireChargeItem::onUseOnBlock(ServerNetworkHandler &owner, ServerPlayer &player, const ItemStack &item,
                                  const Vector3i &blockPosition, int32_t face,
                                  const Vector3f &clickPosition) const {
    (void) item;
    (void) clickPosition;

    if (player.getGameType() == (int32_t) GameType::Adventure)
        return false;

    const Vector3i placement = relativeToFace(blockPosition, face);
    if (!canIgniteAgainst(owner, blockPosition, placement))
        return false;

    if (!FireSystem::ignite(owner, placement))
        return false;

    owner.playLevelSound(LevelSoundEvent::GHAST_FIREBALL, centerOf(placement));
    consumeHeldOne(player);
    return true;
}
