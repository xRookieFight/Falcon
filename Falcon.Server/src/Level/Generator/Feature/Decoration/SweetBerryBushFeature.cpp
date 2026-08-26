#include "Level/Generator/Feature/Decoration/SweetBerryBushFeature.h"

#include "Block/Blocks/VanillaBlocks.h"
#include "Level/Generator/Feature/BlockManager.h"
#include "Level/Generator/Feature/Decoration/DecorationSupport.h"

namespace {

    const BlockState &sweetBerryBushState() {
        static const BlockState state = DecorationSupport::withState(
                VanillaBlocks::SWEET_BERRY_BUSH().toBlockState(), "growth", 7);
        return state;
    }

}

const char *SweetBerryBushFeature::name() const {
    return "minecraft:scatter_sweet_berry_bush_feature";
}

int32_t SweetBerryBushFeature::getBase() const {
    return -66;
}

int32_t SweetBerryBushFeature::getRandom() const {
    return 70;
}

void SweetBerryBushFeature::place(BlockManager &manager, int32_t x, int32_t y, int32_t z) {
    manager.setBlockStateAt(x, y, z, sweetBerryBushState());
}
