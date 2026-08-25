#include "Level/ChunkWorker.h"

#include "Block/Blocks/LiquidBlock.h"
#include "Level/FlatChunkGenerator.h"
#include "Level/LevelStorage.h"

#include <algorithm>
#include <utility>

ChunkWorker::ChunkWorker(const FlatChunkGenerator &generator, LevelStorage &storage)
        : mGenerator(generator), mStorage(storage), mRunning(false), mGeneratedCount(0), mLoadedCount(0),
          mSavedCount(0) {}

ChunkWorker::~ChunkWorker() {
    stop();
}

void ChunkWorker::start(size_t threadCount) {
    if (mRunning.load())
        return;

    const size_t count = std::max<size_t>(1, std::min(threadCount, MAX_THREADS));

    mQueues.clear();
    mQueues.reserve(count);
    for (size_t i = 0; i < count; ++i)
        mQueues.push_back(std::unique_ptr<TaskQueue<ChunkTask>>(new TaskQueue<ChunkTask>()));

    mRunning.store(true);

    mThreads.reserve(count);
    for (size_t i = 0; i < count; ++i)
        mThreads.push_back(std::thread(&ChunkWorker::_run, this, i));
}

void ChunkWorker::stop() {
    if (!mRunning.load())
        return;

    mRunning.store(false);

    for (std::unique_ptr<TaskQueue<ChunkTask>> &queue: mQueues)
        queue->close();

    for (std::thread &thread: mThreads) {
        if (thread.joinable())
            thread.join();
    }

    mThreads.clear();
    mQueues.clear();
    mCompleted.close();
}

size_t ChunkWorker::_queueIndexFor(int32_t chunkX, int32_t chunkZ) const {
    const uint64_t key = ((uint64_t) (uint32_t) chunkX << 32) | (uint32_t) chunkZ;
    return (size_t) ((key * 1099511628211ull) >> 32) % mQueues.size();
}

void ChunkWorker::requestLoad(int32_t chunkX, int32_t chunkZ) {
    if (mQueues.empty())
        return;

    ChunkTask task;
    task.mKind = ChunkTask::Kind::Load;
    task.mX = chunkX;
    task.mZ = chunkZ;
    mQueues[_queueIndexFor(chunkX, chunkZ)]->push(std::move(task));
}

void ChunkWorker::requestSave(std::unique_ptr<LevelChunk> chunk) {
    if (chunk == nullptr || mQueues.empty())
        return;

    ChunkTask task;
    task.mKind = ChunkTask::Kind::Save;
    task.mX = chunk->getX();
    task.mZ = chunk->getZ();
    task.mChunk = std::move(chunk);
    mQueues[_queueIndexFor(task.mX, task.mZ)]->push(std::move(task));
}

std::vector<ChunkLoadResult> ChunkWorker::drainCompleted() {
    return mCompleted.drain();
}

size_t ChunkWorker::getPendingTaskCount() const {
    size_t total = 0;
    for (const std::unique_ptr<TaskQueue<ChunkTask>> &queue: mQueues)
        total += queue->size();

    return total;
}

void ChunkWorker::_processLoad(ChunkTask &task) {
    std::unique_ptr<LevelChunk> chunk(new LevelChunk(task.mX, task.mZ));

    if (mStorage.isOpen() && mStorage.loadChunk(*chunk)) {
        mLoadedCount.fetch_add(1);
    } else {
        mGenerator.generate(*chunk);
        mGeneratedCount.fetch_add(1);
    }

    ChunkLoadResult result;
    result.mX = task.mX;
    result.mZ = task.mZ;
    result.mNetworkSubChunkCount = chunk->getNetworkSubChunkCount();
    result.mNetworkData = chunk->encodeNetwork();

    chunk->forEachBlock([&result](int32_t x, int32_t y, int32_t z, const BlockState &state) {
        const LiquidBlock liquid(state);
        if (!liquid.isLiquid() && !liquid.isBubbleColumn())
            return;

        ChunkFluidCell cell;
        cell.mX = x;
        cell.mY = y;
        cell.mZ = z;
        cell.mTickRate = liquid.getTickRate();
        result.mFluidCells.push_back(cell);
    });

    result.mChunk = std::move(chunk);

    mCompleted.push(std::move(result));
}

void ChunkWorker::_processSave(ChunkTask &task) {
    if (task.mChunk == nullptr || !mStorage.isOpen())
        return;

    if (mStorage.saveChunk(*task.mChunk))
        mSavedCount.fetch_add(1);
}

void ChunkWorker::_run(size_t queueIndex) {
    TaskQueue<ChunkTask> &queue = *mQueues[queueIndex];
    ChunkTask task;

    while (queue.waitPop(task)) {
        if (task.mKind == ChunkTask::Kind::Load)
            _processLoad(task);
        else
            _processSave(task);

        task.mChunk.reset();
    }
}
