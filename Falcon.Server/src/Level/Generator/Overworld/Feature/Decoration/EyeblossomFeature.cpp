#include "Level/Generator/Overworld/Feature/Decoration/EyeblossomFeature.h"

#include "Block/Blocks/VanillaBlocks.h"
#include "Level/Generator/Feature/BlockManager.h"
#include "Level/Generator/Overworld/Feature/Decoration/DecorationSupport.h"
#include "Level/Level.h"

namespace {

    const BlockState &closedEyeblossomState() {
        static const BlockState state = VanillaBlocks::CLOSED_EYEBLOSSOM().toBlockState();
        return state;
    }

}

const char *EyeblossomFeature::name() const {
    return "minecraft:scatter_eyeblossom_feature";
}

int32_t EyeblossomFeature::getBase() const {
    return 10;
}

int32_t EyeblossomFeature::getRandom() const {
    return 0;
}

void EyeblossomFeature::place(BlockManager &manager, int32_t x, int32_t y, int32_t z) {
    manager.setBlockStateAt(x, y, z, closedEyeblossomState());
}

bool EyeblossomFeature::isSupportValid(const BlockState &support, Level &level, int32_t x, int32_t y, int32_t z) const {
    return IFeature::isSupportDirt(support)
           && DecorationSupport::isBiomePaleGarden(DecorationSupport::getBiomeAt(level, x, y, z));
}
