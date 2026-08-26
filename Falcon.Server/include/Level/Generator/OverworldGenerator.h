#pragma once

#include "Level/Generator/Biome/OverworldBiomeResult.h"
#include "Level/Generator/OverworldGeneratorConstants.h"
#include "Level/Generator/OverworldNoiseHolder.h"

#include <cstdint>
#include <memory>
#include <string>

class LevelChunk;
class OverworldSurfaceBuilder;
class SurfaceOverwriteBuilder;
class VanillaOverworldBiomeSource;

class OverworldGenerator {
public:
    explicit OverworldGenerator(int64_t seed);

    ~OverworldGenerator();

    OverworldGenerator(const OverworldGenerator &) = delete;

    OverworldGenerator &operator=(const OverworldGenerator &) = delete;

    void generate(LevelChunk &chunk) const;

    int64_t getSeed() const { return mSeed; }

    int getSpawnY() const { return OverworldGeneratorConstants::SEA_LEVEL + 1; }

    int32_t getAirHash() const { return mAirHash; }

    int32_t pickBiome(int32_t x, int32_t y, int32_t z) const;

    OverworldBiomeResult pickBiomeResult(int32_t x, int32_t y, int32_t z) const;

    static int64_t parseSeed(const std::string &value);

    static int64_t chunkHash(int32_t chunkX, int32_t chunkZ);

private:
    void _generateTerrain(LevelChunk &chunk, DensityChunkCache &chunkCache, int32_t *heightMap) const;

    void _generateBiomeMap(LevelChunk &chunk, DensityChunkCache &chunkCache, const int32_t *heightMap) const;

    int64_t mSeed;
    int32_t mAirHash;
    std::unique_ptr<OverworldNoiseHolder> mNoiseHolder;
    std::unique_ptr<VanillaOverworldBiomeSource> mBiomeSource;
    std::unique_ptr<OverworldSurfaceBuilder> mSurfaceBuilder;
    std::unique_ptr<SurfaceOverwriteBuilder> mSurfaceOverwriteBuilder;
};
