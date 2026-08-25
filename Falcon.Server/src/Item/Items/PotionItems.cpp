#include "Item/Items/PotionItems.h"

#include "Actor/ServerActor.h"
#include "Actor/ServerPlayer.h"
#include "Network/Handler/ServerNetworkHandler.h"
#include "Protocol/Packets/LevelSoundEventPacket.h"
#include "Protocol/Types/StartGameTypes.h"

#include <memory>
#include <string>
#include <utility>

namespace {
    void replaceHeld(ServerPlayer &player, const std::string &replacement) {
        if (player.getGameType() == (int32_t) GameType::Creative)
            return;

        PlayerInventory &inventory = player.getInventory();
        ItemStack held = inventory.getItemInHand();
        if (held.isAir())
            return;

        held.mCount -= 1;
        if (held.mCount <= 0 && replacement.empty()) {
            inventory.setItemInHand(ItemStack::air());
        } else if (held.mCount <= 0) {
            inventory.setItemInHand(ItemStack::air());
        } else {
            inventory.setItemInHand(std::move(held));
        }

        player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory, inventory.getSelectedSlot());
    }

    class ThrownPotionItem : public ItemBehavior {
    public:
        explicit ThrownPotionItem(std::string entityIdentifier) : mEntityIdentifier(std::move(entityIdentifier)) {}

        bool onUse(ServerNetworkHandler &owner, ServerPlayer &player, const ItemStack &item) override {
            ServerActor *projectile = owner.spawnProjectile(player, mEntityIdentifier, 1.5f);
            if (projectile == nullptr)
                return false;

            owner.setProjectilePotionData(projectile->getUniqueId(), item.mDamage);
            owner.playLevelSound(LevelSoundEvent::THROW, player.getPosition(), "minecraft:player");
            replaceHeld(player, std::string());
            return true;
        }

    private:
        std::string mEntityIdentifier;
    };
}

void registerPotionItems(ItemBehaviorRegistry &registry) {
    registry.registerBehavior("minecraft:splash_potion",
                              std::make_unique<ThrownPotionItem>("minecraft:splash_potion"));
    registry.registerBehavior("minecraft:lingering_potion",
                              std::make_unique<ThrownPotionItem>("minecraft:lingering_potion"));
}
