#pragma once

#include "Level/Dimension.h"
#include "Level/Generator/ChunkGenerator.h"

#include <cstdint>
#include <memory>

class DimensionFactory {
public:
    static std::unique_ptr<ChunkGenerator> createGenerator(DimensionType dimension, int64_t seed);
};
