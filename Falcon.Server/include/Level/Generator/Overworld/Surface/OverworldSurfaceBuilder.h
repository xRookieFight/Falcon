#pragma once

#include "Level/Generator/Noise/NormalNoise.h"
#include "Level/Generator/Random/IRandom.h"

class LevelChunk;

class OverworldSurfaceBuilder {
public:
    explicit OverworldSurfaceBuilder(IRandom &random);

    void apply(LevelChunk &chunk) const;

private:
    NormalNoisePtr mNoise;
};
