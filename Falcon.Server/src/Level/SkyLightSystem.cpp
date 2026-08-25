#include "Level/SkyLightSystem.h"

#include "Block/Block.h"
#include "Block/BlockData.h"
#include "Block/Blocks/VanillaBlocks.h"
#include "Level/Level.h"
#include "Level/LevelChunk.h"

#include <algorithm>
#include <cmath>
#include <deque>

namespace {
    const float FULL_CIRCLE = 6.2831855f;
    const float SKY_LIGHT_SCALE = 11.0f;
    const float WEATHER_REDUCTION = 5.0f / 16.0f;

    struct LightNode {
        int32_t mX;
        int32_t mY;
        int32_t mZ;
        int mLight;
    };

    bool blocksSkyLight(const BlockState &state) {
        const Block *block = VanillaBlocks::fromIdentifier(state.mName);
        if (block == nullptr)
            return false;

        const BlockData *data = block->getData();
        if (data == nullptr)
            return false;

        return !data->mTransparent && data->mSolid;
    }

    float clampFloat(float value, float minimum, float maximum) {
        return std::max(minimum, std::min(maximum, value));
    }
}

float SkyLightSystem::calculateCelestialAngle(int64_t time) {
    const int32_t dayTime = (int32_t) (time % 24000);
    float angle = (float) dayTime / 24000.0f - 0.25f;

    if (angle < 0.0f)
        ++angle;
    if (angle > 1.0f)
        --angle;

    const float smoothed = 1.0f - (float) ((std::cos((double) angle * 3.14159265358979323846) + 1.0) / 2.0);
    return angle + (smoothed - angle) / 3.0f;
}

int32_t SkyLightSystem::calculateSkyLightSubtracted(const Level &level) {
    const float rain = level.isRaining() ? 1.0f : 0.0f;
    const float thunder = level.isThundering() ? 1.0f : 0.0f;

    const float rainFactor = 1.0f - rain * WEATHER_REDUCTION;
    const float thunderFactor = 1.0f - thunder * WEATHER_REDUCTION;

    const float angle = calculateCelestialAngle(level.getTime());
    const float brightness = 0.5f + 2.0f * clampFloat(std::cos(angle * FULL_CIRCLE), -0.25f, 0.25f);

    return (int32_t) ((1.0f - brightness * rainFactor * thunderFactor) * SKY_LIGHT_SCALE);
}

void SkyLightSystem::updateHeightAt(LevelChunk &chunk, int x, int z) {
    int32_t height = LevelChunk::MIN_Y;

    for (int index = LevelChunk::SUB_CHUNK_COUNT - 1; index >= 0; --index) {
        if (chunk.getSubChunk(index).isEmpty())
            continue;

        const int32_t baseY = LevelChunk::MIN_Y + index * 16;
        bool found = false;

        for (int32_t offset = 15; offset >= 0; --offset) {
            const int32_t y = baseY + offset;
            if (blocksSkyLight(chunk.getBlock(x, y, z))) {
                height = y + 1;
                found = true;
                break;
            }
        }

        if (found)
            break;
    }

    chunk.setHeight(x, z, height);
}

void SkyLightSystem::computeHeightmap(LevelChunk &chunk) {
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z)
            updateHeightAt(chunk, x, z);
    }
}

void SkyLightSystem::computeChunk(LevelChunk &chunk) {
    chunk.clearSkyLightOnly();

    std::deque<LightNode> pending;

    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            const int32_t height = chunk.getHeight(x, z);

            for (int32_t y = LevelChunk::MAX_Y; y >= height; --y)
                chunk.setSkyLight(x, y, z, MAX_LIGHT);

            if (height > LevelChunk::MIN_Y)
                pending.push_back(LightNode{x, height, z, MAX_LIGHT});
        }
    }

    while (!pending.empty()) {
        const LightNode node = pending.front();
        pending.pop_front();

        const int spread = node.mLight - 1;
        if (spread <= 0)
            continue;

        static const int32_t offsets[6][3] = {
                {1,  0,  0},
                {-1, 0,  0},
                {0,  0,  1},
                {0,  0,  -1},
                {0,  -1, 0},
                {0,  1,  0}
        };

        for (const auto &offset: offsets) {
            const int32_t nextX = node.mX + offset[0];
            const int32_t nextY = node.mY + offset[1];
            const int32_t nextZ = node.mZ + offset[2];

            if (nextX < 0 || nextX > 15 || nextZ < 0 || nextZ > 15)
                continue;
            if (nextY < LevelChunk::MIN_Y || nextY > LevelChunk::MAX_Y)
                continue;
            if (blocksSkyLight(chunk.getBlock((int) nextX, nextY, (int) nextZ)))
                continue;
            if (chunk.getSkyLight((int) nextX, nextY, (int) nextZ) >= spread)
                continue;

            chunk.setSkyLight((int) nextX, nextY, (int) nextZ, spread);
            pending.push_back(LightNode{nextX, nextY, nextZ, spread});
        }
    }
}

void SkyLightSystem::onBlockChanged(Level &level, int32_t x, int32_t y, int32_t z) {
    (void) y;

    LevelChunk *chunk = level.peekChunkPtr(x >> 4, z >> 4);
    if (chunk == nullptr)
        return;

    computeChunk(*chunk);
}
