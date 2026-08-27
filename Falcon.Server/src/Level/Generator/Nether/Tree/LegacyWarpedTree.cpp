#include "Level/Generator/Nether/Tree/LegacyWarpedTree.h"

#include "Block/Blocks/VanillaBlocks.h"

BlockState LegacyWarpedTree::getTrunkBlockState() const {
    return VanillaBlocks::WARPED_STEM().toBlockState();
}

BlockState LegacyWarpedTree::getLeafBlockState() const {
    return VanillaBlocks::WARPED_WART_BLOCK().toBlockState();
}
