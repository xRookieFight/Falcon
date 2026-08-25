#pragma once

#include "Inventory/PlayerInventory.h"
#include "Inventory/CraftingManager.h"
#include "Protocol/Packets/CreativeContentPacket.h"
#include "Protocol/Packets/ItemStackResponsePacket.h"
#include "Protocol/Types/ItemStackRequest.h"

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
