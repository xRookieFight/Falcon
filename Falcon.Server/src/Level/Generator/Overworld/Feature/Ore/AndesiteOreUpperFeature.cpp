#include "Level/Generator/Overworld/Feature/Ore/AndesiteOreUpperFeature.h"

#include "Block/Blocks/VanillaBlocks.h"

namespace {
    const BlockState &andesiteState() {
        static const BlockState state = VanillaBlocks::ANDESITE().toBlockState();
        return state;
    }
}

const BlockState &AndesiteOreUpperFeature::getState(const BlockState &original) const {
    (void) original;
    return andesiteState();
}

const char *AndesiteOreUpperFeature::name() const {
    return "minecraft:overworld_underground_andesite_upper_feature";
}
