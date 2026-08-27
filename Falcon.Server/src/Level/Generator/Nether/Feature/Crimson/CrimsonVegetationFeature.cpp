#include "Level/Generator/Nether/Feature/Crimson/CrimsonVegetationFeature.h"

#include "Block/Blocks/VanillaBlocks.h"
#include "Level/Generator/Nether/NetherBiomeIds.h"

int32_t CrimsonVegetationFeature::getBiomeId() const {
    return NetherBiomeIds::CRIMSON_FOREST;
}

const char *CrimsonVegetationFeature::getNyliumIdentifier() const {
    return "minecraft:crimson_nylium";
}

const BlockState &CrimsonVegetationFeature::getFungusState() const {
    static const BlockState state = VanillaBlocks::CRIMSON_FUNGUS().toBlockState();
    return state;
}

const BlockState &CrimsonVegetationFeature::getRootsState() const {
    static const BlockState state = VanillaBlocks::CRIMSON_ROOTS().toBlockState();
    return state;
}

const char *CrimsonVegetationFeature::name() const {
    return "nether_crimson_grasses";
}
