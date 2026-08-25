#pragma once

#include "block/systems/LiquidBlocksFetch.h"

class ServerPlayer;

class WaterMoveSystem {
public:
    static void tick(ServerPlayer &player, const LiquidContact &contact);
};
