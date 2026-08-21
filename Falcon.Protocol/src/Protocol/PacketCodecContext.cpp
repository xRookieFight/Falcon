#include "Protocol/PacketCodecContext.h"

PacketCodecContext::PacketCodecContext(const BlockDefinitionRegistry &blockDefinitions,
                                       const ItemDefinitionRegistry &itemDefinitions)
        : mBlockDefinitions(blockDefinitions), mItemDefinitions(itemDefinitions) {}
