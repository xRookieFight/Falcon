#include "Level/Level.h"

Level::Level(const std::string &name, int viewDistance)
        : mName(name), mViewDistance(viewDistance), mChunkData(mGenerator.encodeChunk()) {}

Vector3i Level::getSpawnPosition() const {
    return Vector3i(0, mGenerator.getSpawnY(), 0);
}

Vector3f Level::getSpawnPositionForPlayer() const {
    return Vector3f(0.0f, (float) mGenerator.getSpawnY(), 0.0f);
}

int64_t Level::_packPosition(int32_t x, int32_t y, int32_t z) {
    return ((int64_t) (x & 0x3FFFFFF) << 38) | ((int64_t) (z & 0x3FFFFFF) << 12) | (int64_t) (y & 0xFFF);
}

int32_t Level::getBlock(int32_t x, int32_t y, int32_t z) const {
    auto it = mBlockOverrides.find(_packPosition(x, y, z));
    if (it != mBlockOverrides.end())
        return it->second;

    return mGenerator.blockHashAt(y);
}

void Level::setBlock(int32_t x, int32_t y, int32_t z, int32_t blockHash) {
    mBlockOverrides[_packPosition(x, y, z)] = blockHash;
}

std::vector<Level::ChunkPosition> Level::getChunksAround(int32_t centerChunkX, int32_t centerChunkZ) const {
    std::vector<ChunkPosition> chunks;

    const int side = mViewDistance * 2 + 1;
    chunks.reserve((size_t) side * side);

    for (int32_t x = centerChunkX - mViewDistance; x <= centerChunkX + mViewDistance; x++) {
        for (int32_t z = centerChunkZ - mViewDistance; z <= centerChunkZ + mViewDistance; z++)
            chunks.push_back(ChunkPosition{x, z});
    }

    return chunks;
}
