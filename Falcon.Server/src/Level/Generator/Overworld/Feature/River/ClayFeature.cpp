#include "Level/Generator/Overworld/Feature/River/ClayFeature.h"

#include "Block/Blocks/VanillaBlocks.h"

namespace {
    const BlockState &clayState() {
        static const BlockState state = VanillaBlocks::CLAY().toBlockState();
        return state;
    }
}

const BlockState &ClayFeature::getSourceBlock() const {
    return clayState();
}

int32_t ClayFeature::getMinRadius() const {
    return 1;
}

int32_t ClayFeature::getMaxRadius() const {
    return 3;
}

int32_t ClayFeature::getRadiusY() const {
    return 1;
}

int32_t ClayFeature::getBase() const {
    return 1;
}

const char *ClayFeature::name() const {
    return "minecraft:overworld_surface_clay_feature";
}
