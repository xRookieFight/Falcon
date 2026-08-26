#include "Level/Generator/Feature/Tree/SavannaMutatedTreeFeature.h"

#include "Level/Generator/Feature/Tree/FallenTree.h"
#include "Level/Generator/Feature/Tree/LegacyOakTree.h"
#include "Level/Generator/Feature/Tree/SavannaTree.h"

const char *SavannaMutatedTreeFeature::NAME = "minecraft:savanna_mutated_surface_trees_feature";

const char *SavannaMutatedTreeFeature::name() const {
    return NAME;
}

ObjectGeneratorPtr SavannaMutatedTreeFeature::getGenerator(IRandom &random) {
    if (random.nextInt(3) == 0) {
        if (random.nextInt(100) == 0)
            return std::make_shared<FallenTree>();

        return std::make_shared<LegacyOakTree>();
    }

    return std::make_shared<SavannaTree>();
}
