#include "Level/Generator/Feature/Ore/AndesiteOreLowerFeature.h"

#include "Block/Blocks/VanillaBlocks.h"

namespace {
    const BlockState &andesiteState() {
        static const BlockState state = VanillaBlocks::ANDESITE().toBlockState();
        return state;
    }
}

const BlockState &AndesiteOreLowerFeature::getState(const BlockState &original) const {
    (void) original;
    return andesiteState();
}

const char *AndesiteOreLowerFeature::name() const {
    return "minecraft:overworld_underground_andesite_lower_feature";
}
