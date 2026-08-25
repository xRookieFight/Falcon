#pragma once

#include "Level/LevelChunk.h"
#include "Core/NBT/Tag.h"

#include <cstdint>
#include <string>
#include <vector>

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

    bool saveChunk(const LevelChunk &chunk);

    bool loadChunk(LevelChunk &chunk);

    bool saveEntities(int32_t chunkX, int32_t chunkZ, const std::vector<Tag> &entities);

    std::vector<Tag> loadEntities(int32_t chunkX, int32_t chunkZ);

    bool saveBlockEntities(int32_t chunkX, int32_t chunkZ, const std::vector<Tag> &blockEntities);

    std::vector<Tag> loadBlockEntities(int32_t chunkX, int32_t chunkZ);

    bool saveWeather(bool raining, int32_t rainTime, bool thundering, int32_t thunderTime);

    bool loadWeather(bool &raining, int32_t &rainTime, bool &thundering, int32_t &thunderTime);

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
