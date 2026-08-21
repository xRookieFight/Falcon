#pragma once

#include "Core/Math/Vector3f.h"
#include "Core/Math/Vector3i.h"
#include "Level/FlatChunkGenerator.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Level {
public:
    struct ChunkPosition {
        int32_t mX;
        int32_t mZ;
    };

    Level(const std::string &name, int viewDistance);

    const std::string &getName() const { return mName; }

    int getViewDistance() const { return mViewDistance; }

    int getDimensionId() const { return 0; }

    Vector3i getSpawnPosition() const;

    Vector3f getSpawnPositionForPlayer() const;

    std::vector<ChunkPosition> getChunksAround(int32_t centerChunkX, int32_t centerChunkZ) const;

    const std::string &getChunkData() const { return mChunkData; }

    uint32_t getSubChunkCount() const { return FlatChunkGenerator::SUB_CHUNK_COUNT; }

    int32_t getBlock(int32_t x, int32_t y, int32_t z) const;

    void setBlock(int32_t x, int32_t y, int32_t z, int32_t blockHash);

    int32_t getAirHash() const { return mGenerator.getAirHash(); }

private:
    static int64_t _packPosition(int32_t x, int32_t y, int32_t z);

    std::string mName;
    int mViewDistance;
    FlatChunkGenerator mGenerator;
    std::string mChunkData;
    std::unordered_map<int64_t, int32_t> mBlockOverrides;
};
