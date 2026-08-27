#include "Level/Generator/Nether/Feature/Ore/NetherGoldOreFeature.h"

#include "Block/Blocks/VanillaBlocks.h"

const BlockState &NetherGoldOreFeature::getState(const BlockState &original) const {
    (void) original;

    static const BlockState state = VanillaBlocks::NETHER_GOLD_ORE().toBlockState();
    return state;
}

int32_t NetherGoldOreFeature::getClusterCount() const {
    return 10;
}

int32_t NetherGoldOreFeature::getClusterSize() const {
    return 10;
}

int32_t NetherGoldOreFeature::getMinHeight() const {
    return 10;
}

int32_t NetherGoldOreFeature::getMaxHeight() const {
    return 117;
}

OreFeature::ConcentrationType NetherGoldOreFeature::getConcentration() const {
    return ConcentrationType::TRIANGLE;
}

const char *NetherGoldOreFeature::name() const {
    return "nether_nether_gold_ore";
}
