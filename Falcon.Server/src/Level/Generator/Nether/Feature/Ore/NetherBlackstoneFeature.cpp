#include "Level/Generator/Nether/Feature/Ore/NetherBlackstoneFeature.h"

#include "Block/Blocks/VanillaBlocks.h"

const BlockState &NetherBlackstoneFeature::getState() const {
    static const BlockState state = VanillaBlocks::BLACKSTONE().toBlockState();
    return state;
}

const char *NetherBlackstoneFeature::name() const {
    return "nether_blackstone";
}
