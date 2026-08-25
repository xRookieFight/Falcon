#pragma once

#include "Inventory/Container/ContainerManagerModel.h"

class ChestContainerScreenValidator : public ContainerScreenValidatorBase {
};

class ChestContainerManagerModel : public ContainerManagerModel {
public:
    ChestContainerManagerModel();

protected:
    bool openWindow(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3i &position) override;
};
