#include "Level/Generator/Feature/GenerateFeatureRegistry.h"
#include "Level/Generator/Feature/Ore/AndesiteOreLowerFeature.h"
#include "Level/Generator/Feature/Ore/AndesiteOreUpperFeature.h"
#include "Level/Generator/Feature/Ore/CoalOreLowerFeature.h"
#include "Level/Generator/Feature/Ore/CoalOreMountainsFeature.h"
#include "Level/Generator/Feature/Ore/CoalOreUpperFeature.h"
#include "Level/Generator/Feature/Ore/CopperOreDripstoneCavesFeature.h"
#include "Level/Generator/Feature/Ore/CopperOreFeature.h"
#include "Level/Generator/Feature/Ore/DiamondOreBuriedFeature.h"
#include "Level/Generator/Feature/Ore/DiamondOreFeature.h"
#include "Level/Generator/Feature/Ore/DiamondOreLargeFeature.h"
#include "Level/Generator/Feature/Ore/DiamondOreSquareFeature.h"
#include "Level/Generator/Feature/Ore/DioriteOreLowerFeature.h"
#include "Level/Generator/Feature/Ore/DioriteOreUpperFeature.h"
#include "Level/Generator/Feature/Ore/DirtOreFeature.h"
#include "Level/Generator/Feature/Ore/EmeraldOreFeature.h"
#include "Level/Generator/Feature/Ore/GoldOreFeature.h"
#include "Level/Generator/Feature/Ore/GoldOreLowerFeature.h"
#include "Level/Generator/Feature/Ore/GoldOreMesaFeature.h"
#include "Level/Generator/Feature/Ore/GraniteOreLowerFeature.h"
#include "Level/Generator/Feature/Ore/GraniteOreUpperFeature.h"
#include "Level/Generator/Feature/Ore/GravelOreFeature.h"
#include "Level/Generator/Feature/Ore/InfestedOreFeature.h"
#include "Level/Generator/Feature/Ore/IronOreMiddleFeature.h"
#include "Level/Generator/Feature/Ore/IronOreSmallFeature.h"
#include "Level/Generator/Feature/Ore/IronOreUpperFeature.h"
#include "Level/Generator/Feature/Ore/LapisOreBuriedFeature.h"
#include "Level/Generator/Feature/Ore/LapisOreFeature.h"
#include "Level/Generator/Feature/Ore/LegacyEmeraldOreFeature.h"
#include "Level/Generator/Feature/Ore/RedstoneOreFeature.h"
#include "Level/Generator/Feature/Ore/RedstoneOreLowerFeature.h"
#include "Level/Generator/Feature/Ore/TuffOreFeature.h"
#include "Level/Generator/Feature/River/ClayFeature.h"
#include "Level/Generator/Feature/River/GravelFeature.h"
#include "Level/Generator/Feature/River/SandFeature.h"
#include "Level/Generator/Feature/Terrain/CanyonFeature.h"
#include "Level/Generator/Feature/Terrain/CaveFeature.h"
#include "Level/Generator/Feature/Terrain/ExtraUndergroundCaveFeature.h"

#include <memory>

void registerOreFeatures() {
    GenerateFeatureRegistry::registerFeature(std::make_shared<AndesiteOreLowerFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<AndesiteOreUpperFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<CoalOreLowerFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<CoalOreMountainsFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<CoalOreUpperFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<CopperOreDripstoneCavesFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<CopperOreFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<DiamondOreBuriedFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<DiamondOreFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<DiamondOreLargeFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<DiamondOreSquareFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<DioriteOreLowerFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<DioriteOreUpperFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<DirtOreFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<EmeraldOreFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<GoldOreFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<GoldOreLowerFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<GoldOreMesaFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<GraniteOreLowerFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<GraniteOreUpperFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<GravelOreFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<InfestedOreFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<IronOreMiddleFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<IronOreSmallFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<IronOreUpperFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<LapisOreBuriedFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<LapisOreFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<LegacyEmeraldOreFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<RedstoneOreFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<RedstoneOreLowerFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<TuffOreFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<ClayFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<GravelFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<SandFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<CanyonFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<CaveFeature>());
    GenerateFeatureRegistry::registerFeature(std::make_shared<ExtraUndergroundCaveFeature>());
}
