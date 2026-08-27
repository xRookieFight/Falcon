#include "Level/Generator/Overworld/Feature/Decoration/BushFeature.h"

#include "Block/Blocks/VanillaBlocks.h"
#include "Level/Generator/Feature/BlockManager.h"

namespace {

    const BlockState &bushState() {
        static const BlockState state = VanillaBlocks::BUSH().toBlockState();
        return state;
    }

}

const char *BushFeature::name() const {
    return "minecraft:scatter_bush_feature";
}

int32_t BushFeature::getBase() const {
    return 8;
}

int32_t BushFeature::getRandom() const {
    return 0;
}

void BushFeature::place(BlockManager &manager, int32_t x, int32_t y, int32_t z) {
    manager.setBlockStateAt(x, y, z, bushState());
}
