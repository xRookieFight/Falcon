#pragma once

#include <cstdint>

enum class ActorFlag : int {
    OnFire = 0,
    Sneaking = 1,
    Riding = 2,
    Sprinting = 3,
    UsingItem = 4,
    Invisible = 5,
    Baby = 11,
    CanClimb = 19,
    CanWalk = 22,
    Gliding = 32,
    Breathing = 35,
    HasCollision = 48,
    HasGravity = 49,
    SpinAttack = 56,
    Swimming = 57,
    Crawling = 114,
};

class ActorFlags {
public:
    static const int32_t FLAGS_DATA_ID = 0;
    static const int32_t FLAGS_2_DATA_ID = 91;

    void set(ActorFlag flag, bool value);

    bool get(ActorFlag flag) const;

    int64_t getLowBits() const { return mLow; }

    int64_t getHighBits() const { return mHigh; }

    void applyPlayerDefaults();

private:
    int64_t mLow = 0;
    int64_t mHigh = 0;
};
