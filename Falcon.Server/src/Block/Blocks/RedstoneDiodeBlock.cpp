#include "Block/Blocks/RedstoneDiodeBlock.h"

#include "Block/Systems/RedstoneSystem.h"

bool RedstoneRepeaterBlock::matches(const std::string &identifier)
{
    return identifier == "minecraft:unpowered_repeater" || identifier == "minecraft:powered_repeater";
}

bool RedstoneRepeaterBlock::onInteract(ServerNetworkHandler &owner, ServerPlayer &player,
                                       const Vector3i &position, const BlockState &state) const
{
    (void) player;

    RedstoneSystem::onRepeaterActivated(owner, position, state);
    return true;
}

bool RedstoneComparatorBlock::matches(const std::string &identifier)
{
    return identifier == "minecraft:unpowered_comparator" || identifier == "minecraft:powered_comparator";
}

bool RedstoneComparatorBlock::onInteract(ServerNetworkHandler &owner, ServerPlayer &player,
                                         const Vector3i &position, const BlockState &state) const
{
    (void) player;

    RedstoneSystem::onComparatorActivated(owner, position, state);
    return true;
}
