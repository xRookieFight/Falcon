#include "Entity/ItemEntity.h"

ItemEntity::ItemEntity(uint64_t runtimeId, const ItemStack &item) : Entity(runtimeId), mItem(item) {
    mFlags.set(EntityFlag::HasCollision, true);
    mFlags.set(EntityFlag::HasGravity, true);
}

void ItemEntity::tick() {
    if (mPickupDelay > 0)
        mPickupDelay--;

    mAge++;
}
