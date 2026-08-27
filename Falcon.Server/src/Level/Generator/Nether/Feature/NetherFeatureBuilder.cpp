#include "Level/Generator/Nether/Feature/NetherFeatureBuilder.h"

#include "Level/Generator/Feature/ChunkGenerateContext.h"
#include "Level/Generator/Feature/GeneratorRoot.h"
#include "Level/Generator/Feature/IFeature.h"
#include "Level/Generator/Nether/Feature/BasaltDelta/BasaltPillarFeature.h"
#include "Level/Generator/Nether/Feature/BasaltDelta/DeltaFeature.h"
#include "Level/Generator/Nether/Feature/BasaltDelta/DeltaMagmaFeature.h"
#include "Level/Generator/Nether/Feature/Crimson/CrimsonHugeFungusFeature.h"
#include "Level/Generator/Nether/Feature/Crimson/CrimsonVegetationFeature.h"
#include "Level/Generator/Nether/Feature/Crimson/WeepingVinesClusterFeature.h"
#include "Level/Generator/Nether/Feature/GlowStoneFeature.h"
#include "Level/Generator/Nether/Feature/NetherFireFeature.h"
#include "Level/Generator/Nether/Feature/NetherSpringFeature.h"
#include "Level/Generator/Nether/Feature/Ore/AncientDebrisLargeFeature.h"
#include "Level/Generator/Nether/Feature/Ore/AncientDebrisSmallFeature.h"
#include "Level/Generator/Nether/Feature/Ore/MagmaOreFeature.h"
#include "Level/Generator/Nether/Feature/Ore/NetherBlackstoneFeature.h"
#include "Level/Generator/Nether/Feature/Ore/NetherGoldOreFeature.h"
#include "Level/Generator/Nether/Feature/Ore/NetherGravelFeature.h"
#include "Level/Generator/Nether/Feature/Ore/NetherLavaOreFeature.h"
#include "Level/Generator/Nether/Feature/Ore/NetherQuartzOreFeature.h"
#include "Level/Generator/Nether/Feature/Ore/SoulSandOreFeature.h"
#include "Level/Generator/Nether/Feature/Warped/TwistingVinesClusterFeature.h"
#include "Level/Generator/Nether/Feature/Warped/WarpedHugeFungusFeature.h"
#include "Level/Generator/Nether/Feature/Warped/WarpedVegetationFeature.h"
#include "Level/Level.h"
#include "Level/LevelChunk.h"

#include <memory>
#include <vector>

namespace {
    const std::vector<IFeaturePtr> &netherFeatures() {
        static thread_local const std::vector<IFeaturePtr> features = {
                std::make_shared<GlowStoneFeature>(),
                std::make_shared<SoulSandOreFeature>(),
                std::make_shared<MagmaOreFeature>(),
                std::make_shared<NetherLavaOreFeature>(),
                std::make_shared<NetherFireFeature>(),
                std::make_shared<NetherSpringFeature>(),
                std::make_shared<NetherGoldOreFeature>(),
                std::make_shared<AncientDebrisSmallFeature>(),
                std::make_shared<AncientDebrisLargeFeature>(),
                std::make_shared<NetherQuartzOreFeature>(),
                std::make_shared<DeltaFeature>(),
                std::make_shared<BasaltPillarFeature>(),
                std::make_shared<DeltaMagmaFeature>(),
                std::make_shared<CrimsonHugeFungusFeature>(),
                std::make_shared<CrimsonVegetationFeature>(),
                std::make_shared<WeepingVinesClusterFeature>(),
                std::make_shared<WarpedHugeFungusFeature>(),
                std::make_shared<WarpedVegetationFeature>(),
                std::make_shared<TwistingVinesClusterFeature>(),
                std::make_shared<NetherBlackstoneFeature>(),
                std::make_shared<NetherGravelFeature>()
        };

        return features;
    }
}

void NetherFeatureBuilder::apply(Level &level, LevelChunk &chunk, std::vector<GeneratedBlockChange> *overflow) {
    GeneratorRoot root(level);
    root.setOverflowSink(chunk.getX(), chunk.getZ(), overflow);

    ChunkGenerateContext context(level, chunk);

    for (const IFeaturePtr &feature: netherFeatures()) {
        feature->setRoot(&root);
        feature->apply(context);
    }

    root.applySubChunkUpdate();
}
