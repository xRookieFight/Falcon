#include "protocol/types/BlockDefinition.h"

BlockDefinition::BlockDefinition(std::string identifier, int runtimeId, Tag state)
        : mIdentifier(std::move(identifier)), mRuntimeId(runtimeId), mState(std::move(state)) {}
