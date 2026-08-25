#include "block/systems/LavaResetFallDistanceSystem.h"

#include "actor/ServerPlayer.h"

void LavaResetFallDistanceSystem::tick(ServerPlayer &player, const LiquidContact &contact) {
    if (contact.lava)
        player.resetFallDistance();
}
