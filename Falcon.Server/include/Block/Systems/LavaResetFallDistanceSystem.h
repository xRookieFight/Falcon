#pragma once

#include "Block/Systems/LiquidBlocksFetch.h"

class ServerPlayer;

class LavaResetFallDistanceSystem {
public:
    static void tick(ServerPlayer &player, const LiquidContact &contact);
};
