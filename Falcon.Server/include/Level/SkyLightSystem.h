#pragma once

#include <cstdint>

class Level;
class LevelChunk;

class SkyLightSystem {
public:
    static constexpr int MAX_LIGHT = 15;

    static void computeHeightmap(LevelChunk &chunk);

    static void updateHeightAt(LevelChunk &chunk, int x, int z);

    static void computeChunk(LevelChunk &chunk);

    static void onBlockChanged(Level &level, int32_t x, int32_t y, int32_t z);

    static int32_t calculateSkyLightSubtracted(const Level &level);

    static float calculateCelestialAngle(int64_t time);
};
