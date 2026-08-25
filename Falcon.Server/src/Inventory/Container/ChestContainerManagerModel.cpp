#include "Inventory/Container/ChestContainerManagerModel.h"

#include "Actor/ServerPlayer.h"
#include "Inventory/InventoryManager.h"

#include <memory>

ChestContainerManagerModel::ChestContainerManagerModel()
        : ContainerManagerModel(ContainerType::Container,
                                std::make_unique<ChestContainerScreenValidator>()) {}

bool ChestContainerManagerModel::openWindow(ServerNetworkHandler &owner, ServerPlayer &player,
                                            const Vector3i &position) {
    (void) owner;

    return player.getInventoryManager().onClientOpenChest(position);
}
