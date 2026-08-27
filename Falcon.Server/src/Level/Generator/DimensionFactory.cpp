#include "Level/Generator/DimensionFactory.h"

#include "Level/Generator/End/TheEndGenerator.h"
#include "Level/Generator/Nether/NetherGenerator.h"
#include "Level/Generator/Overworld/OverworldGenerator.h"

std::unique_ptr<ChunkGenerator> DimensionFactory::createGenerator(DimensionType dimension, int64_t seed) {
    switch (dimension) {
        case DimensionType::Nether:
            return std::unique_ptr<ChunkGenerator>(new NetherGenerator(seed));
        case DimensionType::TheEnd:
            return std::unique_ptr<ChunkGenerator>(new TheEndGenerator(seed));
        default:
            return std::unique_ptr<ChunkGenerator>(new OverworldGenerator(seed));
    }
}
