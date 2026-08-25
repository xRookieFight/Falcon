#include "level/Level.h"

#include "block/BlockData.h"
#include "block/blocks/VanillaBlocks.h"
#include "core/debug/BedrockLog.h"

#include <algorithm>
#include <utility>

Level::Level(const std::string &name, int viewDistance)
        : mName(name), mViewDistance(viewDistance), mLiquidPhysics(*this) {}

Level &Level::operator=(Level &&other) noexcept {
    if (this == &other)
        return *this;

    if (mChunkWorker != nullptr)
        mChunkWorker->stop();
    mChunkWorker.reset();

    if (other.mChunkWorker != nullptr)
        other.mChunkWorker->stop();
    other.mChunkWorker.reset();

    mName = std::move(other.mName);
    mViewDistance = other.mViewDistance;
    mTime = other.mTime;
    mGenerator = std::move(other.mGenerator);
    mStorage = std::move(other.mStorage);
    mChunks = std::move(other.mChunks);
    mChunkNetworkCache = std::move(other.mChunkNetworkCache);
    mPendingChunks = std::move(other.mPendingChunks);
    mActiveColumns = std::move(other.mActiveColumns);
    mCompletedChunks = std::move(other.mCompletedChunks);
    mLiquidPhysics.moveStateFrom(std::move(other.mLiquidPhysics));
    return *this;
}

bool Level::openStorage(const std::string &worldsDirectory) {
    if (!mStorage.open(worldsDirectory, mName, getDimensionId()))
        return false;

    const Vector3i spawn = getSpawnPosition();
    mStorage.writeLevelDat(mName, spawn.x, spawn.y, spawn.z, 0, 1, 0);
    return true;
}

void Level::saveAll() {
    if (!mStorage.isOpen())
        return;

    const bool async = mChunkWorker != nullptr && mChunkWorker->isRunning();

    size_t saved = 0;
    for (auto &entry: mChunks) {
        if (!entry.second.isDirty())
            continue;

        if (async) {
            mChunkWorker->requestSave(std::unique_ptr<LevelChunk>(new LevelChunk(entry.second)));
            entry.second.clearDirty();
            saved++;
            continue;
        }

        if (mStorage.saveChunk(entry.second)) {
            entry.second.clearDirty();
            saved++;
        }
    }

    if (saved != 0)
        LOG_INFO(LogAreaID::Server, "Saved %zu chunk(s) for level %s", saved, mName.c_str());
}

void Level::closeStorage() {
    saveAll();

    if (mChunkWorker != nullptr)
        mChunkWorker->stop();

    mStorage.close();
}

Vector3i Level::getSpawnPosition() const {
    return Vector3i(0, mGenerator.getSpawnY(), 0);
}

Vector3f Level::getSpawnPositionForPlayer() const {
    return Vector3f(0.0f, (float) mGenerator.getSpawnY(), 0.0f);
}

int64_t Level::_packChunk(int32_t x, int32_t z) {
    return ((int64_t) x << 32) | (uint32_t) z;
}

void Level::_generate(LevelChunk &chunk) {
    mGenerator.generate(chunk);
}

LevelChunk &Level::getChunk(int32_t chunkX, int32_t chunkZ) {
    const int64_t key = _packChunk(chunkX, chunkZ);

    auto it = mChunks.find(key);
    if (it != mChunks.end())
        return it->second;

    LevelChunk chunk(chunkX, chunkZ);

    if (!mStorage.isOpen() || !mStorage.loadChunk(chunk))
        _generate(chunk);

    mPendingChunks.erase(key);

    auto result = mChunks.emplace(key, std::move(chunk));
    mLiquidPhysics.onChunkLoaded(result.first->second);
    return result.first->second;
}

bool Level::isChunkResident(int32_t chunkX, int32_t chunkZ) const {
    return mChunks.find(_packChunk(chunkX, chunkZ)) != mChunks.end();
}

bool Level::isColumnActive(int32_t chunkX, int32_t chunkZ) const {
    return mActiveColumns.find(_packChunk(chunkX, chunkZ)) != mActiveColumns.end();
}

void Level::setActiveColumns(std::vector<int64_t> columns) {
    std::unordered_set<int64_t> next(columns.begin(), columns.end());

    for (int64_t column: columns) {
        if (mActiveColumns.find(column) == mActiveColumns.end())
            mLiquidPhysics.activateColumn((int32_t) (column >> 32), (int32_t) (column & 0xffffffff));
    }

    mActiveColumns.swap(next);
}

void Level::startWorkers(size_t threadCount) {
    if (mChunkWorker == nullptr)
        mChunkWorker.reset(new ChunkWorker(mGenerator, mStorage));

    mChunkWorker->start(threadCount);
}

bool Level::requestChunkAsync(int32_t chunkX, int32_t chunkZ) {
    const int64_t key = _packChunk(chunkX, chunkZ);

    if (mChunks.find(key) != mChunks.end())
        return true;

    if (mChunkWorker == nullptr || !mChunkWorker->isRunning())
        return false;

    if (!mPendingChunks.insert(key).second)
        return false;

    mChunkWorker->requestLoad(chunkX, chunkZ);
    return false;
}

size_t Level::drainCompletedChunks() {
    if (mChunkWorker == nullptr)
        return 0;

    if (mCompletedChunks.empty()) {
        std::vector<ChunkLoadResult> results = mChunkWorker->drainCompleted();
        for (ChunkLoadResult &result: results)
            mCompletedChunks.push_back(std::move(result));
    }

    size_t added = 0;

    while (!mCompletedChunks.empty() && added < MAX_CHUNK_INSERTS_PER_TICK) {
        ChunkLoadResult &result = mCompletedChunks.front();
        const int64_t key = _packChunk(result.mX, result.mZ);
        mPendingChunks.erase(key);

        if (result.mChunk != nullptr && mChunks.find(key) == mChunks.end()) {
            mChunks.emplace(key, std::move(*result.mChunk));
            mChunkNetworkCache[key] = std::move(result.mNetworkData);

            for (const ChunkFluidCell &cell: result.mFluidCells)
                mLiquidPhysics.schedule(Vector3i(cell.mX, cell.mY, cell.mZ), cell.mTickRate);

            added++;
        }

        mCompletedChunks.pop_front();
    }

    return added;
}

std::string Level::getChunkData(int32_t chunkX, int32_t chunkZ) {
    const int64_t key = _packChunk(chunkX, chunkZ);

    auto cached = mChunkNetworkCache.find(key);
    if (cached != mChunkNetworkCache.end())
        return cached->second;

    std::string data = getChunk(chunkX, chunkZ).encodeNetwork();
    mChunkNetworkCache[key] = data;
    return data;
}

int Level::getChunkSubChunkCount(int32_t chunkX, int32_t chunkZ) {
    return getChunk(chunkX, chunkZ).getNetworkSubChunkCount();
}

int32_t Level::getBlock(int32_t x, int32_t y, int32_t z) {
    return getChunk(x >> 4, z >> 4).getBlock(x & 15, y, z & 15).getHash();
}

BlockState Level::getBlockState(int32_t x, int32_t y, int32_t z) {
    return getChunk(x >> 4, z >> 4).getBlock(x & 15, y, z & 15);
}

bool Level::peekBlockState(int32_t x, int32_t y, int32_t z, BlockState &out) {
    const BlockState *state = peekBlockPtr(x, y, z);
    if (state == nullptr)
        return false;

    out = *state;
    return true;
}

const BlockState *Level::peekBlockPtr(int32_t x, int32_t y, int32_t z) {
    if (y < LevelChunk::MIN_Y || y > LevelChunk::MAX_Y)
        return nullptr;

    auto it = mChunks.find(_packChunk(x >> 4, z >> 4));
    if (it == mChunks.end())
        return nullptr;

    return &it->second.getBlock(x & 15, y, z & 15);
}

bool Level::isSolidAt(int32_t x, int32_t y, int32_t z) {
    if (y < LevelChunk::MIN_Y || y > LevelChunk::MAX_Y)
        return false;

    const BlockState &state = getChunk(x >> 4, z >> 4).getBlock(x & 15, y, z & 15);
    if (state.mName == "minecraft:air")
        return false;

    const BlockData *data = BlockDataTable::find(state.mName.c_str());
    if (data == nullptr)
        return true;

    return data->mSolid;
}

void Level::setBlockState(int32_t x, int32_t y, int32_t z, const BlockState &state) {
    if (y < LevelChunk::MIN_Y || y > LevelChunk::MAX_Y)
        return;

    LevelChunk &chunk = getChunk(x >> 4, z >> 4);
    if (chunk.getBlock(x & 15, y, z & 15) == state)
        return;

    chunk.setBlock(x & 15, y, z & 15, state);
    mChunkNetworkCache.erase(_packChunk(x >> 4, z >> 4));
    mLiquidPhysics.onBlockChanged(x, y, z);
}

void Level::setBlock(int32_t x, int32_t y, int32_t z, int32_t blockHash) {
    if (blockHash == mGenerator.getAirHash()) {
        setBlockState(x, y, z, VanillaBlocks::AIR().toBlockState());
        return;
    }

    setBlockState(x, y, z, VanillaBlocks::STONE().toBlockState());
}

LiquidInfo Level::getLiquidInfo(int32_t x, int32_t y, int32_t z) {
    return mLiquidPhysics.getLiquidInfo(x, y, z);
}

Vector3f Level::getLiquidFlowVector(const Vector3i &position) {
    return mLiquidPhysics.getFlowVector(position);
}

void Level::scheduleFluidTick(const Vector3i &position, int64_t delay) {
    mLiquidPhysics.schedule(position, delay);
}

void Level::tickFluids() {
    mLiquidPhysics.tick();
}

std::vector<Level::FluidChange> Level::consumeFluidChanges() {
    return mLiquidPhysics.consumeChanges();
}

std::vector<Level::ChunkPosition> Level::getChunksAround(int32_t centerChunkX, int32_t centerChunkZ) const {
    std::vector<ChunkPosition> chunks;

    const int64_t radiusSquared = (int64_t) mViewDistance * mViewDistance;

    for (int32_t x = -mViewDistance; x <= mViewDistance; x++) {
        for (int32_t z = -mViewDistance; z <= mViewDistance; z++) {
            if ((int64_t) x * x + (int64_t) z * z > radiusSquared)
                continue;

            chunks.push_back(ChunkPosition{centerChunkX + x, centerChunkZ + z});
        }
    }

    std::sort(chunks.begin(), chunks.end(), [centerChunkX, centerChunkZ](const ChunkPosition &left,
                                                                        const ChunkPosition &right) {
        const int64_t leftX = left.mX - centerChunkX;
        const int64_t leftZ = left.mZ - centerChunkZ;
        const int64_t rightX = right.mX - centerChunkX;
        const int64_t rightZ = right.mZ - centerChunkZ;

        return leftX * leftX + leftZ * leftZ < rightX * rightX + rightZ * rightZ;
    });

    return chunks;
}
