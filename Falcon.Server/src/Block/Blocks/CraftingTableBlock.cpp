#include "Block/Blocks/CraftingTableBlock.h"

#include "Actor/ServerPlayer.h"
#include "Network/Handler/ServerNetworkHandler.h"

bool CraftingTableBlock::onInteract(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3i &position) {
    if (!player.getInventoryManager().onClientOpenCraftingTable(position)) {
        return false;
    }

    player.getInventoryManager().syncCraftingTableState(owner.getRecipeOutputs(), owner.getRecipeSourceIndices());
    return true;
}
