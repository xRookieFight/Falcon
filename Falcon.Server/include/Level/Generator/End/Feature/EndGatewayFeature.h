#pragma once

#include "Level/Generator/End/Object/EndGatewayObject.h"
#include "Level/Generator/Feature/IFeature.h"
#include "Level/Generator/Random/SimpleRandom.h"

#include <cstdint>

class TheEndSimplexNoise;

class EndGatewayFeature : public IFeature {
public:
    EndGatewayFeature(int64_t seed, const TheEndSimplexNoise &islandNoise);

    ~EndGatewayFeature() override = default;

    const char *name() const override;

    void apply(ChunkGenerateContext &context) override;

private:
    int64_t mSeed;
    const TheEndSimplexNoise *mIslandNoise;
    SimpleRandom mChunkRandom;
    EndGatewayObject mGatewayObject;
};
