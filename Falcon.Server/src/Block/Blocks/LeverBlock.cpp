#include "Block/Blocks/LeverBlock.h"

#include "Block/Systems/RedstoneSystem.h"

bool LeverBlock::matches(const std::string &identifier)
{
    return identifier == "minecraft:lever";
}

bool LeverBlock::onInteract(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3i &position,
                            const BlockState &state) const
{
    (void) player;

    RedstoneSystem::onLeverActivated(owner, position, state);
    return true;
}
