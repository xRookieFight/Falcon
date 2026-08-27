#include "Level/Generator/Nether/Feature/Warped/WarpedHugeFungusFeature.h"

#include "Level/Generator/Nether/NetherBiomeIds.h"
#include "Level/Generator/Nether/Tree/LegacyWarpedTree.h"

int32_t WarpedHugeFungusFeature::getBiomeId() const {
    return NetherBiomeIds::WARPED_FOREST;
}

const char *WarpedHugeFungusFeature::getNyliumIdentifier() const {
    return "minecraft:warped_nylium";
}

std::shared_ptr<LegacyNetherTree> WarpedHugeFungusFeature::createTree(int32_t treeHeight) const {
    return std::make_shared<LegacyWarpedTree>(treeHeight);
}

const char *WarpedHugeFungusFeature::name() const {
    return "nether_warped_fungi_tree";
}
