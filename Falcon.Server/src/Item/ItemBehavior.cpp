#include "Item/ItemBehavior.h"

#include "Item/Items/ThrowableItems.h"
#include "Item/Items/PotionItems.h"
#include "Item/Items/SpawnEggItem.h"

ItemBehaviorRegistry &ItemBehaviorRegistry::getInstance() {
    static ItemBehaviorRegistry instance;
    return instance;
}

void ItemBehaviorRegistry::registerBehavior(const std::string &identifier, std::unique_ptr<ItemBehavior> behavior) {
    mBehaviors[identifier] = std::move(behavior);
}

ItemBehavior *ItemBehaviorRegistry::find(const std::string &identifier) const {
    const auto it = mBehaviors.find(identifier);
    return it == mBehaviors.end() ? nullptr : it->second.get();
}

void ItemBehaviorRegistry::registerVanilla() {
    registerThrowableItems(*this);
    registerPotionItems(*this);
    registerSpawnEggs(*this);
}
