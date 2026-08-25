#include "Level/FlatChunkGenerator.h"

#include "Block/Blocks/VanillaBlocks.h"
#include "Core/Utility/BinaryStream.h"
#include "Level/LevelChunk.h"
#include "Protocol/BlockStateHasher.h"

#include <vector>

namespace {
    const int BLOCKS_PER_SUB_CHUNK = 4096;
    const unsigned char SUB_CHUNK_VERSION = 9;
    const int32_t PLAINS_BIOME_ID = 1;

    void appendVarInt(std::string &out, int32_t value) {
        BinaryStream stream;
        stream.putVarInt(value);
        out.append(stream.getBuffer());
    }

    void appendLittleEndianInt(std::string &out, uint32_t value) {
        for (int i = 0; i < 4; i++)
            out.push_back((char) ((value >> (i * 8)) & 0xff));
    }

    int bitsPerBlockFor(int paletteSize) {
        if (paletteSize <= 1) return 0;
        if (paletteSize <= 2) return 1;
        if (paletteSize <= 4) return 2;
        if (paletteSize <= 8) return 3;
        if (paletteSize <= 16) return 4;
        if (paletteSize <= 32) return 5;
        if (paletteSize <= 64) return 6;
        if (paletteSize <= 256) return 8;
        return 16;
    }
}

FlatChunkGenerator::FlatChunkGenerator()
        : mAirHash(BlockStateHasher::hash("minecraft:air")),
          mBedrockHash(BlockStateHasher::hash("minecraft:bedrock")),
          mDirtHash(BlockStateHasher::hash("minecraft:dirt")),
          mGrassHash(BlockStateHasher::hash("minecraft:grass_block")) {}

int32_t FlatChunkGenerator::_blockAt(int y) const {
    if (y == BEDROCK_LAYER_Y)
        return mBedrockHash;
    if (y > BEDROCK_LAYER_Y && y <= DIRT_TOP_Y)
        return mDirtHash;
    if (y == SURFACE_Y)
        return mGrassHash;
    return mAirHash;
}

void FlatChunkGenerator::_encodeUniformStorage(std::string &out, int32_t value) const {
    out.push_back((char) ((0 << 1) | 1));
    appendVarInt(out, value);
}

void FlatChunkGenerator::_encodePalettedStorage(std::string &out, const int32_t *blocks, int blockCount,
                                                 const int32_t *palette, int paletteSize) const {
    const int bitsPerBlock = bitsPerBlockFor(paletteSize);

    if (bitsPerBlock == 0) {
        _encodeUniformStorage(out, palette[0]);
        return;
    }

    out.push_back((char) ((bitsPerBlock << 1) | 1));

    const int blocksPerWord = 32 / bitsPerBlock;
    const int wordCount = (blockCount + blocksPerWord - 1) / blocksPerWord;

    int blockIndex = 0;
    for (int word = 0; word < wordCount; word++) {
        uint32_t packed = 0;

        for (int slot = 0; slot < blocksPerWord && blockIndex < blockCount; slot++, blockIndex++) {
            int32_t paletteIndex = 0;
            for (int i = 0; i < paletteSize; i++) {
                if (palette[i] == blocks[blockIndex]) {
                    paletteIndex = i;
                    break;
                }
            }
            packed |= ((uint32_t) paletteIndex) << (slot * bitsPerBlock);
        }

        appendLittleEndianInt(out, packed);
    }

    appendVarInt(out, paletteSize);
    for (int i = 0; i < paletteSize; i++)
        appendVarInt(out, palette[i]);
}

void FlatChunkGenerator::_encodeSubChunk(std::string &out, int subChunkIndex) const {
    out.push_back((char) SUB_CHUNK_VERSION);
    out.push_back((char) 1);
    out.push_back((char) (signed char) subChunkIndex);

    const int baseY = subChunkIndex * 16;

    std::vector<int32_t> palette;
    for (int localY = 0; localY < 16; localY++) {
        const int32_t block = _blockAt(baseY + localY);
        bool known = false;
        for (int32_t entry: palette) {
            if (entry == block) {
                known = true;
                break;
            }
        }
        if (!known)
            palette.push_back(block);
    }

    if (palette.size() == 1) {
        _encodeUniformStorage(out, palette[0]);
        return;
    }

    std::vector<int32_t> blocks(BLOCKS_PER_SUB_CHUNK);
    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            for (int localY = 0; localY < 16; localY++) {
                const int index = (x << 8) | (z << 4) | localY;
                blocks[index] = _blockAt(baseY + localY);
            }
        }
    }

    _encodePalettedStorage(out, blocks.data(), BLOCKS_PER_SUB_CHUNK, palette.data(), (int) palette.size());
}

void FlatChunkGenerator::_encodeBiomes(std::string &out) const {
    for (int i = 0; i < SUB_CHUNK_COUNT; i++)
        _encodeUniformStorage(out, PLAINS_BIOME_ID);
}

std::string FlatChunkGenerator::encodeChunk() const {
    std::string data;

    for (int i = 0; i < SUB_CHUNK_COUNT; i++)
        _encodeSubChunk(data, LOWEST_SUB_CHUNK_INDEX + i);

    _encodeBiomes(data);

    data.push_back((char) 0);

    return data;
}

void FlatChunkGenerator::generate(LevelChunk &chunk) const {
    const BlockState bedrock = VanillaBlocks::BEDROCK().toBlockState();
    const BlockState stone = VanillaBlocks::STONE().toBlockState();
    const BlockState grass = VanillaBlocks::GRASS().toBlockState();

    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            chunk.setBlock(x, LevelChunk::MIN_Y, z, bedrock);

            for (int32_t y = LevelChunk::MIN_Y + 1; y <= DIRT_TOP_Y; y++)
                chunk.setBlock(x, y, z, stone);

            chunk.setBlock(x, SURFACE_Y, z, grass);
        }
    }

    chunk.clearDirty();
}
