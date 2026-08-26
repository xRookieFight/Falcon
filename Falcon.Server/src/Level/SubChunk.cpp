#include "Level/SubChunk.h"

#include "Block/Blocks/VanillaBlocks.h"
#include "Core/Debug/BedrockLog.h"
#include "Scripting/Content/CustomContentRegistry.h"
#include "Core/NBT/NbtIo.h"

SubChunk::SubChunk(int8_t y) : mY(y) {
    mPalette.push_back(BlockState("minecraft:air"));
    mBlocks.assign(BLOCK_COUNT, 0);
    mBiomePalette.push_back(1);
}

uint16_t SubChunk::_biomePaletteIndexFor(uint32_t biomeId) {
    for (size_t i = 0; i < mBiomePalette.size(); i++) {
        if (mBiomePalette[i] == biomeId)
            return (uint16_t) i;
    }

    mBiomePalette.push_back(biomeId);
    return (uint16_t) (mBiomePalette.size() - 1);
}

uint32_t SubChunk::getBiome(int x, int y, int z) const {
    if (mBiomes.empty())
        return mBiomePalette[0];

    return mBiomePalette[mBiomes[_index(x, y, z)]];
}

void SubChunk::setBiome(int x, int y, int z, uint32_t biomeId) {
    const uint16_t index = _biomePaletteIndexFor(biomeId);

    if (mBiomes.empty()) {
        if (index == 0)
            return;

        mBiomes.assign(BLOCK_COUNT, 0);
    }

    mBiomes[_index(x, y, z)] = index;
}

void SubChunk::writeBiomes(BinaryStream &stream, bool persistent) const {
    if (mBiomePalette.size() <= 1) {
        stream.putByte((unsigned char) ((0 << 1) | (persistent ? 0 : 1)));

        if (persistent)
            stream.putLInt((uint32_t) mBiomePalette[0]);
        else
            stream.putVarInt((int32_t) mBiomePalette[0]);

        return;
    }

    const int bitsPerBlock = _bitsPerBlock(mBiomePalette.size());
    const int blocksPerWord = 32 / bitsPerBlock;
    const int wordCount = (BLOCK_COUNT + blocksPerWord - 1) / blocksPerWord;

    stream.putByte((unsigned char) ((bitsPerBlock << 1) | (persistent ? 0 : 1)));

    int blockIndex = 0;
    for (int word = 0; word < wordCount; word++) {
        uint32_t packed = 0;
        for (int slot = 0; slot < blocksPerWord && blockIndex < BLOCK_COUNT; slot++) {
            packed |= ((uint32_t) mBiomes[blockIndex]) << (slot * bitsPerBlock);
            blockIndex++;
        }
        stream.putLInt(packed);
    }

    if (persistent) {
        stream.putLInt((uint32_t) mBiomePalette.size());
        for (uint32_t biomeId: mBiomePalette)
            stream.putLInt(biomeId);
        return;
    }

    stream.putVarInt((int32_t) mBiomePalette.size());
    for (uint32_t biomeId: mBiomePalette)
        stream.putVarInt((int32_t) biomeId);
}

bool SubChunk::readBiomes(ReadOnlyBinaryStream &stream) {
    const unsigned char header = stream.getByte();
    const int bitsPerBlock = header >> 1;

    if (bitsPerBlock == 0) {
        const uint32_t biomeId = stream.getLInt();
        mBiomePalette.assign(1, biomeId);
        mBiomes.clear();
        return true;
    }

    if (bitsPerBlock > 16)
        return false;

    const int blocksPerWord = 32 / bitsPerBlock;
    const int wordCount = (BLOCK_COUNT + blocksPerWord - 1) / blocksPerWord;
    const uint32_t mask = (1u << bitsPerBlock) - 1u;

    mBiomes.assign(BLOCK_COUNT, 0);

    int blockIndex = 0;
    for (int word = 0; word < wordCount; word++) {
        const uint32_t packed = stream.getLInt();
        for (int slot = 0; slot < blocksPerWord && blockIndex < BLOCK_COUNT; slot++) {
            mBiomes[blockIndex] = (uint16_t) ((packed >> (slot * bitsPerBlock)) & mask);
            blockIndex++;
        }
    }

    const uint32_t paletteSize = stream.getLInt();
    if (paletteSize == 0 || paletteSize > BLOCK_COUNT)
        return false;

    mBiomePalette.clear();
    mBiomePalette.reserve(paletteSize);

    for (uint32_t i = 0; i < paletteSize; i++)
        mBiomePalette.push_back(stream.getLInt());

    for (uint16_t &index: mBiomes) {
        if (index >= mBiomePalette.size())
            index = 0;
    }

    return true;
}

int SubChunk::_bitsPerBlock(size_t paletteSize) {
    if (paletteSize <= 2)
        return 1;
    if (paletteSize <= 4)
        return 2;
    if (paletteSize <= 8)
        return 3;
    if (paletteSize <= 16)
        return 4;
    if (paletteSize <= 32)
        return 5;
    if (paletteSize <= 64)
        return 6;
    if (paletteSize <= 256)
        return 8;

    return 16;
}

const BlockState &SubChunk::getBlock(int x, int y, int z) const {
    return mPalette[mBlocks[_index(x, y, z)]];
}

uint16_t SubChunk::_paletteIndexFor(const BlockState &state) {
    for (size_t i = 0; i < mPalette.size(); i++) {
        if (mPalette[i] == state)
            return (uint16_t) i;
    }

    mPalette.push_back(state);
    return (uint16_t) (mPalette.size() - 1);
}

void SubChunk::setBlock(int x, int y, int z, const BlockState &state) {
    mBlocks[_index(x, y, z)] = _paletteIndexFor(state);
}

bool SubChunk::isEmpty() const {
    if (mPalette.size() == 1)
        return mPalette[0].mName == "minecraft:air";

    return false;
}

void SubChunk::_writeStorage(BinaryStream &stream, bool persistent) const {
    const int bitsPerBlock = _bitsPerBlock(mPalette.size());
    const int blocksPerWord = 32 / bitsPerBlock;
    const int wordCount = (BLOCK_COUNT + blocksPerWord - 1) / blocksPerWord;

    stream.putByte((unsigned char) ((bitsPerBlock << 1) | (persistent ? 0 : 1)));

    int blockIndex = 0;
    for (int word = 0; word < wordCount; word++) {
        uint32_t packed = 0;
        for (int slot = 0; slot < blocksPerWord && blockIndex < BLOCK_COUNT; slot++) {
            packed |= ((uint32_t) mBlocks[blockIndex]) << (slot * bitsPerBlock);
            blockIndex++;
        }
        stream.putLInt(packed);
    }

    if (persistent) {
        stream.putLInt((uint32_t) mPalette.size());
        for (const BlockState &state: mPalette)
            NbtIo::writeTag(stream, state.toNbt(), NbtVariant::LittleEndian);
        return;
    }

    stream.putVarInt((int32_t) mPalette.size());
    for (const BlockState &state: mPalette)
        stream.putVarInt(state.getHash());
}

void SubChunk::writeNetwork(BinaryStream &stream) const {
    stream.putByte(STORAGE_VERSION);
    stream.putByte(LAYER_COUNT);
    stream.putByte((unsigned char) (signed char) mY);

    _writeStorage(stream, false);
    _writeEmptyStorage(stream, false);
}

void SubChunk::_writeEmptyStorage(BinaryStream &stream, bool persistent) const {
    stream.putByte((unsigned char) ((1 << 1) | (persistent ? 0 : 1)));

    for (int word = 0; word < 128; word++)
        stream.putLInt(0);

    const BlockState air;

    if (persistent) {
        stream.putLInt(1);
        NbtIo::writeTag(stream, air.toNbt(), NbtVariant::LittleEndian);
        return;
    }

    stream.putVarInt(1);
    stream.putVarInt(air.getHash());
}

void SubChunk::writePersistent(BinaryStream &stream) const {
    stream.putByte(STORAGE_VERSION);
    stream.putByte(LAYER_COUNT);
    stream.putByte((unsigned char) (signed char) mY);

    _writeStorage(stream, true);
    _writeEmptyStorage(stream, true);
}

bool SubChunk::readPersistent(ReadOnlyBinaryStream &stream, bool *replacedUnknown) {
    const unsigned char version = stream.getByte();
    const unsigned char layers = stream.getByte();

    if (version >= 9)
        mY = (int8_t) stream.getByte();

    if (layers == 0)
        return false;

    const unsigned char header = stream.getByte();
    const int bitsPerBlock = header >> 1;
    if (bitsPerBlock <= 0 || bitsPerBlock > 16)
        return false;

    const int blocksPerWord = 32 / bitsPerBlock;
    const int wordCount = (BLOCK_COUNT + blocksPerWord - 1) / blocksPerWord;
    const uint32_t mask = bitsPerBlock == 32 ? 0xffffffffu : ((1u << bitsPerBlock) - 1u);

    mBlocks.assign(BLOCK_COUNT, 0);

    int blockIndex = 0;
    for (int word = 0; word < wordCount; word++) {
        const uint32_t packed = stream.getLInt();
        for (int slot = 0; slot < blocksPerWord && blockIndex < BLOCK_COUNT; slot++) {
            mBlocks[blockIndex] = (uint16_t) ((packed >> (slot * bitsPerBlock)) & mask);
            blockIndex++;
        }
    }

    const uint32_t paletteSize = stream.getLInt();
    if (paletteSize == 0 || paletteSize > BLOCK_COUNT)
        return false;

    mPalette.clear();
    mPalette.reserve(paletteSize);

    for (uint32_t i = 0; i < paletteSize; i++) {
        const Tag tag = NbtIo::readTag(stream, NbtVariant::LittleEndian);
        const std::string name = tag.getString("name", "minecraft:air");
        const Tag *states = tag.get("states");

        if (VanillaBlocks::fromIdentifier(name) == nullptr &&
            !CustomContentRegistry::getInstance().isCustomBlock(name)) {
            LOG_WARN(LogAreaID::Level, "Replaced unknown block %s with air while loading chunk data", name.c_str());
            mPalette.push_back(BlockState("minecraft:air"));
            if (replacedUnknown != nullptr)
                *replacedUnknown = true;
            continue;
        }

        mPalette.push_back(states == nullptr ? BlockState(name) : BlockState(name, *states));
    }

    for (uint16_t &index: mBlocks) {
        if (index >= mPalette.size())
            index = 0;
    }

    return true;
}
