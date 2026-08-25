#pragma once

#include "core/math/Vector3f.h"

class ServerNetworkHandler;
class ServerPlayer;
class ItemActor;
class ItemStack;

class ItemActorHandler {
public:
    static ItemActor *dropItem(ServerNetworkHandler &owner, const Vector3f &position, const ItemStack &item,
                               const Vector3f &motion, int pickupDelay);

    static void sendItemActorsTo(ServerNetworkHandler &owner, ServerPlayer &player);

    static void tickItemActors(ServerNetworkHandler &owner);
};
