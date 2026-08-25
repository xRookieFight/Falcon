#include "Inventory/Container/FurnaceContainerManagerModel.h"

#include "Actor/ServerPlayer.h"
#include "Block/Blocks/FurnaceBlock.h"
#include "Inventory/InventoryManager.h"

#include <memory>

FurnaceContainerManagerModel::FurnaceContainerManagerModel(const BlockState &state)
        : ContainerManagerModel(FurnaceBlock::containerType(state),
                                std::make_unique<FurnaceContainerScreenValidator>()),
          mState(state) {}

bool FurnaceContainerManagerModel::openWindow(ServerNetworkHandler &owner, ServerPlayer &player,
                                              const Vector3i &position) {
    (void) owner;

    return player.getInventoryManager().onClientOpenFurnace(position, FurnaceBlock::kind(mState), mType);
}
