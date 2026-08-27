#pragma once

#include "Level/Generator/End/Object/EndIslandObject.h"
#include "Level/Generator/Feature/IFeature.h"
#include "Level/Generator/Random/SimpleRandom.h"

#include <cstdint>

class TheEndSimplexNoise;

class EndIslandFeature : public IFeature {
public:
    EndIslandFeature(int64_t seed, const TheEndSimplexNoise &islandNoise);

    ~EndIslandFeature() override = default;

    const char *name() const override;

    void apply(ChunkGenerateContext &context) override;

private:
    int64_t mSeed;
    const TheEndSimplexNoise *mIslandNoise;
    SimpleRandom mChunkRandom;
    EndIslandObject mIslandObject;
};
