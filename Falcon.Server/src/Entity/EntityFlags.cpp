#include "Entity/EntityFlags.h"

void EntityFlags::set(EntityFlag flag, bool value) {
    const int bit = (int) flag;
    int64_t &target = bit < 64 ? mLow : mHigh;
    const int64_t mask = (int64_t) 1 << (bit % 64);

    if (value)
        target |= mask;
    else
        target &= ~mask;
}

bool EntityFlags::get(EntityFlag flag) const {
    const int bit = (int) flag;
    const int64_t source = bit < 64 ? mLow : mHigh;
    return (source & ((int64_t) 1 << (bit % 64))) != 0;
}

void EntityFlags::applyPlayerDefaults() {
    set(EntityFlag::CanWalk, true);
    set(EntityFlag::CanClimb, true);
    set(EntityFlag::Breathing, true);
    set(EntityFlag::HasCollision, true);
    set(EntityFlag::HasGravity, true);
}
