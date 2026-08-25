#pragma once

#include "Level/SubChunk.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class LevelChunk {
public:
    static const int SUB_CHUNK_COUNT = 24;
    static const int LOWEST_SUB_CHUNK_Y = -4;
    static const int MIN_Y = LOWEST_SUB_CHUNK_Y * 16;
    static const int MAX_Y = MIN_Y + SUB_CHUNK_COUNT * 16 - 1;
    static const uint8_t STORAGE_VERSION = 40;

    LevelChunk(int32_t x, int32_t z);

    int32_t getX() const { return mX; }

    int32_t getZ() const { return mZ; }

    bool isDirty() const { return mDirty; }

    void clearDirty() { mDirty = false; }

    void markDirty() { mDirty = true; }

    const BlockState &getBlock(int x, int32_t y, int z) const;

    void setBlock(int x, int32_t y, int z, const BlockState &state);

    void forEachBlock(const std::function<void(int32_t, int32_t, int32_t, const BlockState &)> &callback) const;

    void setBiome(uint32_t biomeId) { mBiomeId = biomeId; }

    uint32_t getBiome() const { return mBiomeId; }

    std::string encodeNetwork() const;

    std::string encodeBiomes(int sectionCount) const;

    int getNetworkSubChunkCount() const;

    SubChunk &getSubChunk(int index) { return mSubChunks[index]; }

    const SubChunk &getSubChunk(int index) const { return mSubChunks[index]; }

    static int subChunkIndexFor(int32_t y) { return (y - MIN_Y) >> 4; }

    static const int32_t UNKNOWN_HEIGHT = -32768;

    int32_t getHeight(int x, int z) const;

    void setHeight(int x, int z, int32_t height);

    bool hasHeight(int x, int z) const;

    int getSkyLight(int x, int32_t y, int z) const;

    void setSkyLight(int x, int32_t y, int z, int value);

    bool hasSkyLight() const { return !mSkyLight.empty(); }

    bool hasHeightmap() const { return !mHeightmap.empty(); }

    void clearSkyLight();

    void clearSkyLightOnly() { mSkyLight.clear(); }

private:
    static int _lightIndex(int x, int32_t y, int z);
    int32_t mX;
    int32_t mZ;
    uint32_t mBiomeId;
    bool mDirty;
    std::vector<SubChunk> mSubChunks;
    std::vector<int16_t> mHeightmap;
    std::vector<uint8_t> mSkyLight;
};
