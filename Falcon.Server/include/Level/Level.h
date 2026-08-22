#pragma once

#include "Core/Math/Vector3f.h"
#include "Core/Math/Vector3i.h"
#include "Level/Chunk.h"
#include "Level/FlatChunkGenerator.h"
#include "Level/LevelStorage.h"

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

    bool openStorage(const std::string &worldsDirectory);

    void saveAll();

    void closeStorage();

    Vector3i getSpawnPosition() const;

    Vector3f getSpawnPositionForPlayer() const;

    std::vector<ChunkPosition> getChunksAround(int32_t centerChunkX, int32_t centerChunkZ) const;

    Chunk &getChunk(int32_t chunkX, int32_t chunkZ);

    std::string getChunkData(int32_t chunkX, int32_t chunkZ);

    int getChunkSubChunkCount(int32_t chunkX, int32_t chunkZ);

    uint32_t getSubChunkCount() const { return Chunk::SUB_CHUNK_COUNT; }

    int32_t getBlock(int32_t x, int32_t y, int32_t z);

    bool isSolidAt(int32_t x, int32_t y, int32_t z);

    void setBlock(int32_t x, int32_t y, int32_t z, int32_t blockHash);

    void setBlockState(int32_t x, int32_t y, int32_t z, const BlockState &state);

    int32_t getAirHash() const { return mGenerator.getAirHash(); }

private:
    static int64_t _packChunk(int32_t x, int32_t z);

    void _generate(Chunk &chunk);

    std::string mName;
    int mViewDistance;
    FlatChunkGenerator mGenerator;
    LevelStorage mStorage;
    std::unordered_map<int64_t, Chunk> mChunks;
};
