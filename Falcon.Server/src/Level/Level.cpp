#include "Level/Level.h"

#include "Block/BlockData.h"
#include "Block/Blocks/VanillaBlocks.h"
#include "Core/Debug/BedrockLog.h"

#include <algorithm>

Level::Level(const std::string &name, int viewDistance)
        : mName(name), mViewDistance(viewDistance) {}

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

    size_t saved = 0;
    for (auto &entry: mChunks) {
        if (!entry.second.isDirty())
            continue;

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

void Level::_generate(Chunk &chunk) {
    const BlockState bedrock = VanillaBlocks::BEDROCK().toBlockState();
    const BlockState stone = VanillaBlocks::STONE().toBlockState();
    const BlockState grass = VanillaBlocks::GRASS().toBlockState();

    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            chunk.setBlock(x, Chunk::MIN_Y, z, bedrock);

            for (int32_t y = Chunk::MIN_Y + 1; y <= FlatChunkGenerator::DIRT_TOP_Y; y++)
                chunk.setBlock(x, y, z, stone);

            chunk.setBlock(x, FlatChunkGenerator::SURFACE_Y, z, grass);
        }
    }

    chunk.clearDirty();
}

Chunk &Level::getChunk(int32_t chunkX, int32_t chunkZ) {
    const int64_t key = _packChunk(chunkX, chunkZ);

    auto it = mChunks.find(key);
    if (it != mChunks.end())
        return it->second;

    Chunk chunk(chunkX, chunkZ);

    if (!mStorage.isOpen() || !mStorage.loadChunk(chunk))
        _generate(chunk);

    return mChunks.emplace(key, std::move(chunk)).first->second;
}

std::string Level::getChunkData(int32_t chunkX, int32_t chunkZ) {
    return getChunk(chunkX, chunkZ).encodeNetwork();
}

int Level::getChunkSubChunkCount(int32_t chunkX, int32_t chunkZ) {
    return getChunk(chunkX, chunkZ).getNetworkSubChunkCount();
}

int32_t Level::getBlock(int32_t x, int32_t y, int32_t z) {
    return getChunk(x >> 4, z >> 4).getBlock(x & 15, y, z & 15).mHash;
}

BlockState Level::getBlockState(int32_t x, int32_t y, int32_t z) {
    return getChunk(x >> 4, z >> 4).getBlock(x & 15, y, z & 15);
}

bool Level::isSolidAt(int32_t x, int32_t y, int32_t z) {
    if (y < Chunk::MIN_Y || y > Chunk::MAX_Y)
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
    getChunk(x >> 4, z >> 4).setBlock(x & 15, y, z & 15, state);
}

void Level::setBlock(int32_t x, int32_t y, int32_t z, int32_t blockHash) {
    if (blockHash == mGenerator.getAirHash()) {
        setBlockState(x, y, z, VanillaBlocks::AIR().toBlockState());
        return;
    }

    setBlockState(x, y, z, VanillaBlocks::STONE().toBlockState());
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
