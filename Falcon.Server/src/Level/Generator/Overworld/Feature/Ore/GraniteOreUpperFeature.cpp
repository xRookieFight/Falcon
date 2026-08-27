#include "Level/Generator/Overworld/Feature/Ore/GraniteOreUpperFeature.h"

#include "Block/Blocks/VanillaBlocks.h"

namespace {
    const BlockState &graniteState() {
        static const BlockState state = VanillaBlocks::GRANITE().toBlockState();
        return state;
    }
}

const BlockState &GraniteOreUpperFeature::getState(const BlockState &original) const {
    (void) original;
    return graniteState();
}

int32_t GraniteOreUpperFeature::getClusterCount() const {
    return 6;
}

int32_t GraniteOreUpperFeature::getClusterSize() const {
    return 64;
}

int32_t GraniteOreUpperFeature::getMinHeight() const {
    return 64;
}

int32_t GraniteOreUpperFeature::getMaxHeight() const {
    return 128;
}

bool GraniteOreUpperFeature::isRare() const {
    return true;
}

const char *GraniteOreUpperFeature::name() const {
    return "minecraft:overworld_underground_granite_upper_feature";
}
