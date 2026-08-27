#include "Level/Generator/End/Feature/TheEndFeatureBuilder.h"

#include "Level/Generator/End/Feature/ChorusPlantFeature.h"
#include "Level/Generator/End/Feature/EndGatewayFeature.h"
#include "Level/Generator/End/Feature/EndIslandFeature.h"
#include "Level/Generator/End/Feature/EndPodiumFeature.h"
#include "Level/Generator/End/Feature/SpikeFeature.h"
#include "Level/Generator/End/TheEndNoiseHolder.h"
#include "Level/Generator/Feature/ChunkGenerateContext.h"
#include "Level/Generator/Feature/GeneratorRoot.h"
#include "Level/Level.h"
#include "Level/LevelChunk.h"

#include <memory>
#include <vector>

void TheEndFeatureBuilder::apply(Level &level, LevelChunk &chunk, const TheEndNoiseHolder &noiseHolder,
                                 std::vector<GeneratedBlockChange> *overflow) {
    const int64_t seed = level.getSeed();
    const TheEndSimplexNoise &islandNoise = noiseHolder.getDecorationIslandNoise();

    std::vector<IFeaturePtr> features;
    features.push_back(std::make_shared<SpikeFeature>(seed));
    features.push_back(std::make_shared<EndPodiumFeature>());
    features.push_back(std::make_shared<ChorusPlantFeature>(seed, islandNoise));
    features.push_back(std::make_shared<EndGatewayFeature>(seed, islandNoise));
    features.push_back(std::make_shared<EndIslandFeature>(seed, islandNoise));

    GeneratorRoot root(level);
    root.setOverflowSink(chunk.getX(), chunk.getZ(), overflow);

    ChunkGenerateContext context(level, chunk);

    for (const IFeaturePtr &feature: features) {
        feature->setRoot(&root);
        feature->apply(context);
    }

    root.applySubChunkUpdate();
}
