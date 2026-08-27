#include "Level/Generator/Nether/Tree/LegacyCrimsonTree.h"

#include "Block/Blocks/VanillaBlocks.h"

BlockState LegacyCrimsonTree::getTrunkBlockState() const {
    return VanillaBlocks::CRIMSON_STEM().toBlockState();
}

BlockState LegacyCrimsonTree::getLeafBlockState() const {
    return VanillaBlocks::NETHER_WART_BLOCK().toBlockState();
}
