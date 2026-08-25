#pragma once

#include "Protocol/Types/BlockDefinitionRegistry.h"
#include "Protocol/Types/ItemDefinitionRegistry.h"

class PacketCodecContext {
public:
    PacketCodecContext(const BlockDefinitionRegistry &blockDefinitions,
                       const ItemDefinitionRegistry &itemDefinitions);

    const BlockDefinitionRegistry &getBlockDefinitions() const { return mBlockDefinitions; }

    const ItemDefinitionRegistry &getItemDefinitions() const { return mItemDefinitions; }

private:
    const BlockDefinitionRegistry &mBlockDefinitions;
    const ItemDefinitionRegistry &mItemDefinitions;
};
