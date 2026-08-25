#include "actor/ItemActor.h"

ItemActor::ItemActor(uint64_t runtimeId, const ItemStack &item) : Actor(runtimeId), mItem(item) {
    mFlags.set(ActorFlag::HasCollision, true);
    mFlags.set(ActorFlag::HasGravity, true);
}

void ItemActor::tick() {
    if (mPickupDelay > 0)
        mPickupDelay--;

    mAge++;
}
