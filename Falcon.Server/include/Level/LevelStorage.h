#pragma once

#include "Level/Chunk.h"

#include <cstdint>
#include <string>

namespace leveldb {
    class DB;
}

enum class LevelDbTag : unsigned char {
    Data3D = '+',
    Version = ',',
    Data2D = '-',
    SubChunkPrefix = '/',
    BlockEntities = '1',
    Entities = '2',
    FinalizedState = '6',
    LegacyVersion = 'v'
};

class LevelStorage {
public:
    LevelStorage();

    ~LevelStorage();

    LevelStorage(const LevelStorage &) = delete;

    LevelStorage &operator=(const LevelStorage &) = delete;

    LevelStorage(LevelStorage &&other) noexcept;

    LevelStorage &operator=(LevelStorage &&other) noexcept;

    bool open(const std::string &worldsDirectory, const std::string &levelName, int dimensionId = 0);

    void close();

    bool isOpen() const { return mDb != nullptr; }

    bool saveChunk(const Chunk &chunk);

    bool loadChunk(Chunk &chunk);

    void writeLevelDat(const std::string &levelName, int32_t spawnX, int32_t spawnY, int32_t spawnZ,
                       int32_t gameType, int32_t difficulty, int64_t seed) const;

private:
    std::string _makeKey(int32_t chunkX, int32_t chunkZ, LevelDbTag tag) const;

    std::string _makeSubChunkKey(int32_t chunkX, int32_t chunkZ, int8_t subY) const;

    static void _appendLInt(std::string &out, int32_t value);

    leveldb::DB *mDb;
    std::string mPath;
    int mDimensionId;
};
