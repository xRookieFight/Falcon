#pragma once

#include "inventory/PlayerInventory.h"
#include "inventory/CraftingManager.h"
#include "protocol/packets/CreativeContentPacket.h"
#include "protocol/packets/ItemStackResponsePacket.h"
#include "protocol/types/ItemStackRequest.h"

#include <cstdint>
#include <vector>

class ItemStackRequestHandler {
public:
    static const int RESULT_OK = 0;
    static const int RESULT_ERROR = 1;

    static ItemStackResponseEntry execute(PlayerInventory &inventory, const ItemStackRequest &request,
                                          const std::vector<CreativeItemData> &creativeItems,
                                          const std::vector<ItemStack> &recipeOutputs,
                                          const std::vector<uint32_t> &recipeSourceIndices,
                                          bool craftingTableOpen,
                                          bool furnaceOpen,
                                          std::vector<ItemStack> *outDroppedItems = nullptr);
};
