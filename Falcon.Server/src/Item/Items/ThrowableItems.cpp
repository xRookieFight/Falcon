#include "Item/Items/ThrowableItems.h"

#include "Actor/ServerActor.h"
#include "Actor/ServerPlayer.h"
#include "Network/Handler/ServerNetworkHandler.h"
#include "Protocol/Packets/LevelSoundEventPacket.h"
#include "Protocol/Types/StartGameTypes.h"

#include <memory>
#include <utility>

namespace {
    void consumeHeldOne(ServerNetworkHandler &owner, ServerPlayer &player) {
        (void) owner;
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

        player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory, inventory.getSelectedSlot());
    }
}

ThrowableItem::ThrowableItem(std::string entityIdentifier, float throwForce, int32_t cooldownTicks)
        : mEntityIdentifier(std::move(entityIdentifier)), mThrowForce(throwForce), mCooldownTicks(cooldownTicks) {}

bool ThrowableItem::onUse(ServerNetworkHandler &owner, ServerPlayer &player, const ItemStack &item) {
    if (player.hasItemCooldown(item, owner.getCurrentTick()))
        return true;

    ServerActor *projectile = owner.spawnProjectile(player, mEntityIdentifier, mThrowForce);
    if (projectile == nullptr)
        return false;

    owner.playLevelSound(LevelSoundEvent::THROW, player.getPosition(), "minecraft:player");

    if (mCooldownTicks > 0)
        player.startItemCooldown(item, owner.getCurrentTick(), mCooldownTicks);

    consumeHeldOne(owner, player);
    return true;
}

bool isThrownProjectile(const std::string &identifier) {
    return identifier == "minecraft:snowball" || identifier == "minecraft:egg" ||
           identifier == "minecraft:ender_pearl" || identifier == "minecraft:xp_bottle" ||
           identifier == "minecraft:splash_potion" || identifier == "minecraft:lingering_potion" ||
           identifier == "minecraft:wind_charge_projectile" || identifier == "minecraft:eye_of_ender_signal";
}

void registerThrowableItems(ItemBehaviorRegistry &registry) {
    registry.registerBehavior("minecraft:snowball",
                              std::make_unique<ThrowableItem>("minecraft:snowball", 1.5f, 0));
    registry.registerBehavior("minecraft:egg",
                              std::make_unique<ThrowableItem>("minecraft:egg", 1.5f, 0));
    registry.registerBehavior("minecraft:ender_pearl",
                              std::make_unique<ThrowableItem>("minecraft:ender_pearl", 1.5f, 20));
    registry.registerBehavior("minecraft:experience_bottle",
                              std::make_unique<ThrowableItem>("minecraft:xp_bottle", 1.0f, 0));
    registry.registerBehavior("minecraft:splash_potion",
                              std::make_unique<ThrowableItem>("minecraft:splash_potion", 1.5f, 0));
    registry.registerBehavior("minecraft:lingering_potion",
                              std::make_unique<ThrowableItem>("minecraft:lingering_potion", 1.5f, 0));
    registry.registerBehavior("minecraft:wind_charge",
                              std::make_unique<ThrowableItem>("minecraft:wind_charge_projectile", 1.5f, 10));
    registry.registerBehavior("minecraft:ender_eye",
                              std::make_unique<ThrowableItem>("minecraft:eye_of_ender_signal", 1.2f, 0));
}
