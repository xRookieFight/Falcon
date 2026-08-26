#include "Level/Generator/Feature/Tree/JungleEdgeTreeFeature.h"

#include "Level/Generator/Feature/Tree/FallenTree.h"
#include "Level/Generator/Feature/Tree/FancyOakTree.h"
#include "Level/Generator/Feature/Tree/JungleTree.h"
#include "Level/Generator/Feature/Tree/TreeBiomeTags.h"

const char *JungleEdgeTreeFeature::NAME = "minecraft:legacy:jungle_edge_tree_feature";

const char *JungleEdgeTreeFeature::name() const {
    return NAME;
}

ObjectGeneratorPtr JungleEdgeTreeFeature::getGenerator(IRandom &random) {
    const int32_t choice = random.nextInt(5);

    if (choice == 0 || choice == 1) {
        if (random.nextInt(100) == 0)
            return std::make_shared<FallenTree>(TreeWoodType::JUNGLE);

        return std::make_shared<JungleTree>(7, 8);
    }

    return std::make_shared<FancyOakTree>();
}

bool JungleEdgeTreeFeature::canSpawnHere(int32_t biomeId) const {
    return TreeBiomeTags::isEdge(biomeId);
}

int32_t JungleEdgeTreeFeature::getMin() const {
    return 1;
}

int32_t JungleEdgeTreeFeature::getMax() const {
    return 2;
}
