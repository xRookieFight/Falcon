#pragma once

#include "Item/ItemBehavior.h"

#include <string>

class ThrowableItem : public ItemBehavior {
public:
    ThrowableItem(std::string entityIdentifier, float throwForce, int32_t cooldownTicks);

    bool onUse(ServerNetworkHandler &owner, ServerPlayer &player, const ItemStack &item) override;

private:
    std::string mEntityIdentifier;
    float mThrowForce;
    int32_t mCooldownTicks;
};

void registerThrowableItems(ItemBehaviorRegistry &registry);

bool isThrownProjectile(const std::string &identifier);
