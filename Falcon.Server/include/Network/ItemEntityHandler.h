#pragma once

#include "Core/Math/Vector3f.h"

class ServerNetworkHandler;
class ServerPlayer;
class ItemEntity;
class ItemStack;

class ItemEntityHandler {
public:
    static ItemEntity *dropItem(ServerNetworkHandler &owner, const Vector3f &position, const ItemStack &item,
                                const Vector3f &motion, int pickupDelay);

    static void sendItemEntitiesTo(ServerNetworkHandler &owner, ServerPlayer &player);

    static void tickItemEntities(ServerNetworkHandler &owner);
};
