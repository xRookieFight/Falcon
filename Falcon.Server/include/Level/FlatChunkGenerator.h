#pragma once

#include <cstdint>
#include <string>

class FlatChunkGenerator {
public:
    static const int SUB_CHUNK_COUNT = 24;
    static const int LOWEST_SUB_CHUNK_INDEX = -4;
    static const int MIN_WORLD_Y = -64;

    static const int BEDROCK_LAYER_Y = MIN_WORLD_Y;
    static const int DIRT_TOP_Y = 66;
    static const int SURFACE_Y = 67;

    FlatChunkGenerator();

    std::string encodeChunk() const;

    int getSpawnY() const { return SURFACE_Y + 1; }

    int32_t getAirHash() const { return mAirHash; }

    int32_t blockHashAt(int y) const { return _blockAt(y); }

private:
    void _encodeSubChunk(std::string &out, int subChunkIndex) const;

    void _encodeUniformStorage(std::string &out, int32_t value) const;

    void _encodePalettedStorage(std::string &out, const int32_t *blocks, int blockCount,
                                const int32_t *palette, int paletteSize) const;

    void _encodeBiomes(std::string &out) const;

    int32_t _blockAt(int y) const;

    int32_t mAirHash;
    int32_t mBedrockHash;
    int32_t mDirtHash;
    int32_t mGrassHash;
};
