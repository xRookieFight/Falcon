#pragma once

#include "Block/Systems/LiquidPhysicsSystem.h"
#include "Core/Math/Vector3f.h"
#include "Core/Math/Vector3i.h"
#include "Level/LevelChunk.h"
#include "Level/ChunkWorker.h"
#include "Level/FlatChunkGenerator.h"
#include "Level/LevelStorage.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Level {
public:
    struct ChunkPosition {
        int32_t mX;
        int32_t mZ;
    };

    Level(const std::string &name, int viewDistance);

    Level &operator=(Level &&other) noexcept;

    const std::string &getName() const { return mName; }

    int getViewDistance() const { return mViewDistance; }

    int getDimensionId() const { return 0; }

    int64_t getTime() const { return mTime; }

    int64_t getDayTime() const { return mTime % 24000; }

    void setTime(int64_t time) { mTime = time < 0 ? 0 : time; }

    void addTime(int64_t time) { setTime(mTime + time); }

    void tickTime() { ++mTime; }

    bool openStorage(const std::string &worldsDirectory);

    void saveAll();

    void closeStorage();

    Vector3i getSpawnPosition() const;

    Vector3f getSpawnPositionForPlayer() const;

    std::vector<ChunkPosition> getChunksAround(int32_t centerChunkX, int32_t centerChunkZ) const;

    LevelChunk &getChunk(int32_t chunkX, int32_t chunkZ);

    bool isChunkResident(int32_t chunkX, int32_t chunkZ) const;

    bool requestChunkAsync(int32_t chunkX, int32_t chunkZ);

    size_t drainCompletedChunks();

    void startWorkers(size_t threadCount);

    const ChunkWorker *getChunkWorker() const { return mChunkWorker.get(); }

    std::string getChunkData(int32_t chunkX, int32_t chunkZ);

    int getChunkSubChunkCount(int32_t chunkX, int32_t chunkZ);

    uint32_t getSubChunkCount() const { return LevelChunk::SUB_CHUNK_COUNT; }

    size_t getLoadedChunkCount() const { return mChunks.size(); }

    int32_t getBlock(int32_t x, int32_t y, int32_t z);

    BlockState getBlockState(int32_t x, int32_t y, int32_t z);

    bool peekBlockState(int32_t x, int32_t y, int32_t z, BlockState &out);

    const BlockState *peekBlockPtr(int32_t x, int32_t y, int32_t z);

    bool isSolidAt(int32_t x, int32_t y, int32_t z);

    void setBlock(int32_t x, int32_t y, int32_t z, int32_t blockHash);

    void setBlockState(int32_t x, int32_t y, int32_t z, const BlockState &state);

    LiquidInfo getLiquidInfo(int32_t x, int32_t y, int32_t z);

    Vector3f getLiquidFlowVector(const Vector3i &position);

    void scheduleFluidTick(const Vector3i &position, int64_t delay = 1);

    void tickFluids();

    void setFluidTimeBudgetMs(double milliseconds) { mLiquidPhysics.setTimeBudgetMs(milliseconds); }

    void setActiveColumns(std::vector<int64_t> columns);

    bool isColumnActive(int32_t chunkX, int32_t chunkZ) const;

    size_t getScheduledFluidCount() const { return mLiquidPhysics.getScheduledCount(); }

    size_t getDeferredFluidCount() const { return mLiquidPhysics.getLastDeferredCount(); }

    size_t getLastFluidProcessedCount() const { return mLiquidPhysics.getLastProcessedCount(); }

    using FluidChange = LiquidChange;

    std::vector<FluidChange> consumeFluidChanges();

    int32_t getAirHash() const { return mGenerator.getAirHash(); }

private:
    static int64_t _packChunk(int32_t x, int32_t z);

    void _generate(LevelChunk &chunk);

    std::string mName;
    int mViewDistance;
    FlatChunkGenerator mGenerator;
    LevelStorage mStorage;
    LiquidPhysicsSystem mLiquidPhysics;
    std::unordered_map<int64_t, LevelChunk> mChunks;
    static const size_t MAX_CHUNK_INSERTS_PER_TICK = 8;

    std::unordered_map<int64_t, std::string> mChunkNetworkCache;
    std::unordered_set<int64_t> mPendingChunks;
    std::unordered_set<int64_t> mActiveColumns;
    std::deque<ChunkLoadResult> mCompletedChunks;
    std::unique_ptr<ChunkWorker> mChunkWorker;
    int64_t mTime = 0;
};
