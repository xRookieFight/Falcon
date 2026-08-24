#include "Block/Systems/LavaResetFallDistanceSystem.h"

#include "Actor/ServerPlayer.h"

void LavaResetFallDistanceSystem::tick(ServerPlayer &player, const LiquidContact &contact) {
    if (contact.lava)
        player.resetFallDistance();
}
