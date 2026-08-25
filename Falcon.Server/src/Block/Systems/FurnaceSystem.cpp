#include "block/systems/FurnaceSystem.h"

#include "actor/ServerPlayer.h"
#include "inventory/InventoryManager.h"

void FurnaceSystem::tick(ServerNetworkHandler &owner, ServerPlayer &player) {
    player.getInventoryManager().tickFurnace(owner);
}

void FurnaceSystem::tickStored(ServerNetworkHandler &owner) {
    InventoryManager::tickStoredFurnaces(owner);
}
