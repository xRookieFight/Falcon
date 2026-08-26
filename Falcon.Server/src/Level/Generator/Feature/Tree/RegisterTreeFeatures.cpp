#include "Level/Generator/Feature/GenerateFeatureRegistry.h"
#include "Level/Generator/Feature/Tree/BambooJungleTreeFeature.h"
#include "Level/Generator/Feature/Tree/BirchForestMutatedTreeFeature.h"
#include "Level/Generator/Feature/Tree/BirchForestTreeFeature.h"
#include "Level/Generator/Feature/Tree/CherryTreeFeature.h"
#include "Level/Generator/Feature/Tree/FlowerForestTreeFeature.h"
#include "Level/Generator/Feature/Tree/ForestTreeFeature.h"
#include "Level/Generator/Feature/Tree/GroveTreeFeature.h"
#include "Level/Generator/Feature/Tree/IceSurfaceTreeFeature.h"
#include "Level/Generator/Feature/Tree/JungleBushFeature.h"
#include "Level/Generator/Feature/Tree/JungleEdgeTreeFeature.h"
#include "Level/Generator/Feature/Tree/JungleTreeFeature.h"
#include "Level/Generator/Feature/Tree/MangroveTreeFeature.h"
#include "Level/Generator/Feature/Tree/MeadowTreeFeature.h"
#include "Level/Generator/Feature/Tree/MegaTaigaTreeFeature.h"
#include "Level/Generator/Feature/Tree/MesaPlateauStoneTreeFeature.h"
#include "Level/Generator/Feature/Tree/MesaTreeFeature.h"
#include "Level/Generator/Feature/Tree/MushroomIslandHugeMushroomFeature.h"
#include "Level/Generator/Feature/Tree/PaleGardenTreeFeature.h"
#include "Level/Generator/Feature/Tree/PlainsTreeFeature.h"
#include "Level/Generator/Feature/Tree/RoofedForestTreeFeature.h"
#include "Level/Generator/Feature/Tree/SavannaMutatedTreeFeature.h"
#include "Level/Generator/Feature/Tree/SavannaTreeFeature.h"
#include "Level/Generator/Feature/Tree/SwampTreeFeature.h"
#include "Level/Generator/Feature/Tree/TaigaTreeFeature.h"

void registerTreeFeatures() {
    GenerateFeatureRegistry::registerFeature(std::make_shared<BambooJungleTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<BirchForestMutatedTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<BirchForestTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<CherryTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<FlowerForestTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<ForestTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<GroveTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<IceSurfaceTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<JungleBushFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<JungleEdgeTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<JungleTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<MangroveTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<MeadowTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<MegaTaigaTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<MesaPlateauStoneTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<MesaTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<MushroomIslandHugeMushroomFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<PaleGardenTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<PlainsTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<RoofedForestTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<SavannaMutatedTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<SavannaTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<SwampTreeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<TaigaTreeFeature>());
}
