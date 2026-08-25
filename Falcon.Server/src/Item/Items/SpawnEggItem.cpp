#include "Item/Items/SpawnEggItem.h"

#include "Actor/ServerPlayer.h"
#include "Actor/VanillaActorTable.h"
#include "Network/Handler/ServerNetworkHandler.h"
#include "Protocol/Types/ItemDefinition.h"
#include "Protocol/Types/StartGameTypes.h"

#include <memory>
#include <string>

namespace {
    const std::string SPAWN_EGG_SUFFIX = "_spawn_egg";

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

        player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory, inventory.getSelectedSlot());
    }

    class SpawnEggItem : public ItemBehavior {
    public:
        bool onUseOnBlock(ServerNetworkHandler &owner, ServerPlayer &player, const ItemStack &item,
                          const Vector3i &blockPosition, int32_t face, const Vector3f &clickPosition) override {
            (void) clickPosition;

            if (item.mDefinition == nullptr)
                return false;

            const std::string &identifier = item.mDefinition->getIdentifier();
            if (identifier.size() <= SPAWN_EGG_SUFFIX.size() ||
                identifier.compare(identifier.size() - SPAWN_EGG_SUFFIX.size(), SPAWN_EGG_SUFFIX.size(),
                                   SPAWN_EGG_SUFFIX) != 0)
                return false;

            const std::string entityIdentifier = identifier.substr(0, identifier.size() - SPAWN_EGG_SUFFIX.size());

            Vector3f spawnPosition((float) blockPosition.x + 0.5f, (float) blockPosition.y,
                                   (float) blockPosition.z + 0.5f);
            switch (face) {
                case 0:
                    spawnPosition.y -= 1.0f;
                    break;
                case 1:
                    spawnPosition.y += 1.0f;
                    break;
                case 2:
                    spawnPosition.z -= 1.0f;
                    break;
                case 3:
                    spawnPosition.z += 1.0f;
                    break;
                case 4:
                    spawnPosition.x -= 1.0f;
                    break;
                case 5:
                    spawnPosition.x += 1.0f;
                    break;
                default:
                    spawnPosition.y += 1.0f;
                    break;
            }

            if (owner.spawnActor(entityIdentifier, spawnPosition) == nullptr)
                return false;

            consumeHeldOne(player);
            return true;
        }
    };
}

void registerSpawnEggs(ItemBehaviorRegistry &registry) {
    const char *const *identifiers = VanillaActorTable::getIdentifiers();
    const size_t count = VanillaActorTable::getCount();

    for (size_t index = 0; index < count; ++index) {
        registry.registerBehavior(std::string(identifiers[index]) + SPAWN_EGG_SUFFIX,
                                  std::make_unique<SpawnEggItem>());
    }
}
