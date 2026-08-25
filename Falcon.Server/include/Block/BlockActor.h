#pragma once

#include "Block/BlockState.h"

class BlockActor {
public:
    BlockActor() = default;
    explicit BlockActor(const BlockState &state) : mState(state) {}

    const BlockState &getState() const noexcept { return mState; }

    void setState(const BlockState &state) { mState = state; }

private:
    BlockState mState;
};
