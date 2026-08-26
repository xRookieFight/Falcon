#include "Level/Generator/Feature/Tree/MesaTreeFeature.h"

#include "Level/Generator/Feature/Tree/FallenTree.h"
#include "Level/Generator/Feature/Tree/LegacyOakTree.h"
#include "Level/Generator/Feature/Tree/TreeBiomeTags.h"

const char *MesaTreeFeature::NAME = "minecraft:mesa_tree_feature";

const char *MesaTreeFeature::name() const {
    return NAME;
}

TreeGeneratorPtr MesaTreeFeature::getGenerator(IRandom &random) {
    if (random.nextInt(100) == 0)
        return std::make_shared<FallenTree>();

    return std::make_shared<LegacyOakTree>();
}

bool MesaTreeFeature::canSpawnHere(int32_t biomeId) const {
    return TreeBiomeTags::isMesa(biomeId);
}
