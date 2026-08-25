#pragma once

#include "core/nbt/Tag.h"

#include <string>

class BlockDefinition {
public:
    BlockDefinition(std::string identifier, int runtimeId, Tag state);

    const std::string &getIdentifier() const { return mIdentifier; }

    int getRuntimeId() const { return mRuntimeId; }

    const Tag &getState() const { return mState; }

private:
    std::string mIdentifier;
    int mRuntimeId;
    Tag mState;
};
