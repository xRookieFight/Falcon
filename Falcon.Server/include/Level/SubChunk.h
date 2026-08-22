#pragma once

#include "Core/Utility/BinaryStream.h"
#include "Level/BlockState.h"

#include <cstdint>
#include <vector>

class SubChunk {
public:
    static const int BLOCK_COUNT = 4096;
    static const uint8_t STORAGE_VERSION = 9;
    static const uint8_t LAYER_COUNT = 2;

    explicit SubChunk(int8_t y);

    int8_t getY() const { return mY; }

    const BlockState &getBlock(int x, int y, int z) const;

    void setBlock(int x, int y, int z, const BlockState &state);

    bool isEmpty() const;

    void writeNetwork(BinaryStream &stream) const;

    void writePersistent(BinaryStream &stream) const;

    bool readPersistent(ReadOnlyBinaryStream &stream);

private:
    static int _index(int x, int y, int z) { return (x << 8) | (z << 4) | y; }

    static int _bitsPerBlock(size_t paletteSize);

    void _writeStorage(BinaryStream &stream, bool persistent) const;

    void _writeEmptyStorage(BinaryStream &stream, bool persistent) const;

    uint16_t _paletteIndexFor(const BlockState &state);

    int8_t mY;
    std::vector<BlockState> mPalette;
    std::vector<uint16_t> mBlocks;
};
