#include "Level/Chunk.h"

Chunk::Chunk(int32_t x, int32_t z) : mX(x), mZ(z), mBiomeId(1), mDirty(false) {
    mSubChunks.reserve(SUB_CHUNK_COUNT);
    for (int i = 0; i < SUB_CHUNK_COUNT; i++)
        mSubChunks.push_back(SubChunk((int8_t) (LOWEST_SUB_CHUNK_Y + i)));
}

const BlockState &Chunk::getBlock(int x, int32_t y, int z) const {
    static const BlockState air;

    if (y < MIN_Y || y > MAX_Y)
        return air;

    return mSubChunks[subChunkIndexFor(y)].getBlock(x, y & 15, z);
}

void Chunk::setBlock(int x, int32_t y, int z, const BlockState &state) {
    if (y < MIN_Y || y > MAX_Y)
        return;

    mSubChunks[subChunkIndexFor(y)].setBlock(x, y & 15, z, state);
    mDirty = true;
}

void Chunk::forEachBlock(const std::function<void(int32_t, int32_t, int32_t, const BlockState &)> &callback) const {
    for (int32_t y = MIN_Y; y <= MAX_Y; ++y) {
        for (int32_t x = 0; x < 16; ++x) {
            for (int32_t z = 0; z < 16; ++z)
                callback(mX * 16 + x, y, mZ * 16 + z, getBlock(x, y, z));
        }
    }
}

int Chunk::getNetworkSubChunkCount() const {
    for (int i = SUB_CHUNK_COUNT - 1; i >= 0; i--) {
        if (!mSubChunks[i].isEmpty())
            return i + 1;
    }

    return 0;
}

std::string Chunk::encodeBiomes(int sectionCount) const {
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

std::string Chunk::encodeNetwork() const {
    BinaryStream stream;

    const int sectionCount = getNetworkSubChunkCount();

    for (int i = 0; i < sectionCount; i++)
        mSubChunks[i].writeNetwork(stream);

    stream.put(encodeBiomes(sectionCount));
    stream.putByte(0);

    return stream.getBuffer();
}
