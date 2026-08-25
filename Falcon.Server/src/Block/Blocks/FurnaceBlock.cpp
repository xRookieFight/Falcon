#include "Block/Blocks/FurnaceBlock.h"

#include "Actor/ServerPlayer.h"

bool FurnaceBlock::matches(const BlockState &state) {
    return state.mName == "minecraft:furnace" || state.mName == "minecraft:lit_furnace"
           || state.mName == "minecraft:blast_furnace" || state.mName == "minecraft:lit_blast_furnace"
           || state.mName == "minecraft:smoker" || state.mName == "minecraft:lit_smoker";
}

FurnaceKind FurnaceBlock::kind(const BlockState &state) {
    if (state.mName == "minecraft:blast_furnace" || state.mName == "minecraft:lit_blast_furnace") {
        return FurnaceKind::BlastFurnace;
    }
    if (state.mName == "minecraft:smoker" || state.mName == "minecraft:lit_smoker") {
        return FurnaceKind::Smoker;
    }
    return FurnaceKind::Furnace;
}

ContainerType FurnaceBlock::containerType(const BlockState &state) {
    switch (kind(state)) {
        case FurnaceKind::BlastFurnace:
            return ContainerType::BlastFurnace;
        case FurnaceKind::Smoker:
            return ContainerType::Smoker;
        default:
            return ContainerType::Furnace;
    }
}

bool FurnaceBlock::onInteract(ServerNetworkHandler &owner, ServerPlayer &player,
                               const Vector3i &position, const BlockState &state) {
    (void) owner;
    return player.getInventoryManager().onClientOpenFurnace(position, kind(state), containerType(state));
}
