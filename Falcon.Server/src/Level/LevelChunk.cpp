#include "Level/LevelChunk.h"

LevelChunk::LevelChunk(int32_t x, int32_t z) : mX(x), mZ(z), mBiomeId(1), mDirty(false) {
    mSubChunks.reserve(SUB_CHUNK_COUNT);
    for (int i = 0; i < SUB_CHUNK_COUNT; i++)
        mSubChunks.push_back(SubChunk((int8_t) (LOWEST_SUB_CHUNK_Y + i)));
}

int LevelChunk::_lightIndex(int x, int32_t y, int z) {
    return (((y - MIN_Y) << 8) | (z << 4) | x);
}

int32_t LevelChunk::getHeight(int x, int z) const {
    if (mHeightmap.empty())
        return MIN_Y;

    const int16_t height = mHeightmap[(size_t) ((z << 4) | x)];
    return height == (int16_t) UNKNOWN_HEIGHT ? MIN_Y : height;
}

bool LevelChunk::hasHeight(int x, int z) const {
    if (mHeightmap.empty())
        return false;

    return mHeightmap[(size_t) ((z << 4) | x)] != (int16_t) UNKNOWN_HEIGHT;
}

void LevelChunk::setHeight(int x, int z, int32_t height) {
    if (mHeightmap.empty())
        mHeightmap.assign(256, (int16_t) UNKNOWN_HEIGHT);

    mHeightmap[(size_t) ((z << 4) | x)] = (int16_t) height;
}

int LevelChunk::getSkyLight(int x, int32_t y, int z) const {
    if (mSkyLight.empty() || y < MIN_Y || y > MAX_Y)
        return 0;

    return mSkyLight[(size_t) _lightIndex(x, y, z)];
}

void LevelChunk::setSkyLight(int x, int32_t y, int z, int value) {
    if (y < MIN_Y || y > MAX_Y)
        return;

    if (mSkyLight.empty())
        mSkyLight.assign((size_t) (SUB_CHUNK_COUNT * 16 * 256), 0);

    mSkyLight[(size_t) _lightIndex(x, y, z)] = (uint8_t) value;
}

void LevelChunk::clearSkyLight() {
    mSkyLight.clear();
    mHeightmap.clear();
}

const BlockState &LevelChunk::getBlock(int x, int32_t y, int z) const {
    static const BlockState air;

    if (y < MIN_Y || y > MAX_Y)
        return air;

    return mSubChunks[subChunkIndexFor(y)].getBlock(x, y & 15, z);
}

void LevelChunk::setBlock(int x, int32_t y, int z, const BlockState &state) {
    if (y < MIN_Y || y > MAX_Y)
        return;

    mSubChunks[subChunkIndexFor(y)].setBlock(x, y & 15, z, state);
    mDirty = true;
}

void LevelChunk::forEachBlock(const std::function<void(int32_t, int32_t, int32_t, const BlockState &)> &callback) const {
    for (int32_t y = MIN_Y; y <= MAX_Y; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            for (int32_t z = 0; z < 16; ++z)
                callback(mX * 16 + x, y, mZ * 16 + z, getBlock(x, y, z));
        }
    }
}

int LevelChunk::getNetworkSubChunkCount() const {
    for (int i = SUB_CHUNK_COUNT - 1; i >= 0; i--) {
        if (!mSubChunks[i].isEmpty())
            return i + 1;
    }

    return 0;
}

std::string LevelChunk::encodeBiomes(int sectionCount) const {
    BinaryStream stream;

    for (int i = 0; i < sectionCount; i++) {
        stream.putByte((unsigned char) ((1 << 1) | 1));

        for (int word = 0; word < 128; word++)
            stream.putLInt(0);

        stream.putVarInt(1);
        stream.putVarInt((int32_t) mBiomeId);
    }

    return stream.getBuffer();
}

std::string LevelChunk::encodeNetwork() const {
    BinaryStream stream;

    const int sectionCount = getNetworkSubChunkCount();

    for (int i = 0; i < sectionCount; i++)
        mSubChunks[i].writeNetwork(stream);

    stream.put(encodeBiomes(sectionCount));
    stream.putByte(0);

    return stream.getBuffer();
}
