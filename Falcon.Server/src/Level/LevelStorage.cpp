#include "level/LevelStorage.h"

#include "core/debug/BedrockLog.h"
#include "core/nbt/NbtIo.h"
#include "core/utility/BinaryStream.h"
#include "core/utility/ReadOnlyBinaryStream.h"

#include <filesystem>
#include <fstream>
#include <utility>

#include <leveldb/db.h>
#include <leveldb/options.h>
#include <leveldb/write_batch.h>
#include <leveldb/filter_policy.h>
#include <leveldb/cache.h>

LevelStorage::LevelStorage() : mDb(nullptr), mDimensionId(0) {}

LevelStorage::~LevelStorage() {
    close();
}

LevelStorage::LevelStorage(LevelStorage &&other) noexcept
        : mDb(other.mDb), mPath(std::move(other.mPath)), mDimensionId(other.mDimensionId) {
    other.mDb = nullptr;
}

LevelStorage &LevelStorage::operator=(LevelStorage &&other) noexcept {
    if (this == &other)
        return *this;

    close();

    mDb = other.mDb;
    mPath = std::move(other.mPath);
    mDimensionId = other.mDimensionId;
    other.mDb = nullptr;

    return *this;
}

void LevelStorage::_appendLInt(std::string &out, int32_t value) {
    out.push_back((char) (value & 0xff));
    out.push_back((char) ((value >> 8) & 0xff));
    out.push_back((char) ((value >> 16) & 0xff));
    out.push_back((char) ((value >> 24) & 0xff));
}

std::string LevelStorage::_makeKey(int32_t chunkX, int32_t chunkZ, LevelDbTag tag) const {
    std::string key;
    _appendLInt(key, chunkX);
    _appendLInt(key, chunkZ);

    if (mDimensionId != 0)
        _appendLInt(key, mDimensionId);

    key.push_back((char) tag);
    return key;
}

std::string LevelStorage::_makeSubChunkKey(int32_t chunkX, int32_t chunkZ, int8_t subY) const {
    std::string key = _makeKey(chunkX, chunkZ, LevelDbTag::SubChunkPrefix);
    key.push_back((char) subY);
    return key;
}

bool LevelStorage::open(const std::string &worldsDirectory, const std::string &levelName, int dimensionId) {
    close();

    mDimensionId = dimensionId;

    std::error_code error;
    const std::filesystem::path root = std::filesystem::path(worldsDirectory) / levelName;
    std::filesystem::create_directories(root / "db", error);

    mPath = (root / "db").string();

    LOG_INFO(LogAreaID::Server, "Opening level '%s'", mPath.c_str());

    leveldb::Options options;
    options.create_if_missing = true;
    options.compression = leveldb::kZlibRawCompression;
    options.block_size = 163840;
    options.write_buffer_size = 4 * 1024 * 1024;
    options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    options.block_cache = leveldb::NewLRUCache(8 * 1024 * 1024);

    leveldb::DB *db = nullptr;
    const leveldb::Status status = leveldb::DB::Open(options, mPath, &db);

    if (!status.ok()) {
        LOG_ERROR(LogAreaID::Server, "Could not open level database %s: %s", mPath.c_str(),
                  status.ToString().c_str());
        return false;
    }

    mDb = db;

    const std::filesystem::path nameFile = root / "levelname.txt";
    if (!std::filesystem::exists(nameFile, error)) {
        std::ofstream file(nameFile);
        if (file.is_open())
            file << levelName;
    }

    return true;
}

void LevelStorage::close() {
    if (mDb == nullptr)
        return;

    delete mDb;
    mDb = nullptr;
}

bool LevelStorage::saveChunk(const LevelChunk &chunk) {
    if (mDb == nullptr)
        return false;

    leveldb::WriteBatch batch;

    const std::string version(1, (char) LevelChunk::STORAGE_VERSION);
    batch.Put(_makeKey(chunk.getX(), chunk.getZ(), LevelDbTag::Version), version);

    std::string finalized;
    _appendLInt(finalized, 2);
    batch.Put(_makeKey(chunk.getX(), chunk.getZ(), LevelDbTag::FinalizedState), finalized);

    for (int i = 0; i < LevelChunk::SUB_CHUNK_COUNT; i++) {
        const SubChunk &subChunk = chunk.getSubChunk(i);

        BinaryStream stream;
        subChunk.writePersistent(stream);

        batch.Put(_makeSubChunkKey(chunk.getX(), chunk.getZ(), subChunk.getY()), stream.getBuffer());
    }

    BinaryStream heightAndBiomes;
    for (int i = 0; i < 256; i++) {
        heightAndBiomes.putByte(0);
        heightAndBiomes.putByte(0);
    }
    heightAndBiomes.put(chunk.encodeBiomes(LevelChunk::SUB_CHUNK_COUNT));
    batch.Put(_makeKey(chunk.getX(), chunk.getZ(), LevelDbTag::Data3D), heightAndBiomes.getBuffer());

    const leveldb::Status status = mDb->Write(leveldb::WriteOptions(), &batch);
    if (!status.ok()) {
        LOG_WARN(LogAreaID::Server, "Could not save chunk %d %d: %s", chunk.getX(), chunk.getZ(),
                 status.ToString().c_str());
        return false;
    }

    return true;
}

bool LevelStorage::loadChunk(LevelChunk &chunk) {
    if (mDb == nullptr)
        return false;

    std::string version;
    leveldb::Status status = mDb->Get(leveldb::ReadOptions(),
                                      _makeKey(chunk.getX(), chunk.getZ(), LevelDbTag::Version), &version);

    if (!status.ok()) {
        status = mDb->Get(leveldb::ReadOptions(),
                          _makeKey(chunk.getX(), chunk.getZ(), LevelDbTag::LegacyVersion), &version);
    }

    if (!status.ok())
        return false;

    bool loadedAny = false;

    for (int i = 0; i < LevelChunk::SUB_CHUNK_COUNT; i++) {
        const int8_t subY = (int8_t) (LevelChunk::LOWEST_SUB_CHUNK_Y + i);

        std::string data;
        if (!mDb->Get(leveldb::ReadOptions(), _makeSubChunkKey(chunk.getX(), chunk.getZ(), subY), &data).ok())
            continue;

        ReadOnlyBinaryStream stream(data);

        try {
            if (chunk.getSubChunk(i).readPersistent(stream))
                loadedAny = true;
        } catch (const BinaryDataException &exception) {
            LOG_WARN(LogAreaID::Server, "Malformed sub chunk %d %d %d: %s", chunk.getX(), chunk.getZ(),
                     (int) subY, exception.what());
        }
    }

    chunk.clearDirty();
    return loadedAny;
}

void LevelStorage::writeLevelDat(const std::string &levelName, int32_t spawnX, int32_t spawnY, int32_t spawnZ,
                                 int32_t gameType, int32_t difficulty, int64_t seed) const {
    if (mPath.empty())
        return;

    const std::filesystem::path root = std::filesystem::path(mPath).parent_path();

    Tag data = Tag::ofCompound();
    data.putString("LevelName", levelName);
    data.putInt("SpawnX", spawnX);
    data.putInt("SpawnY", spawnY);
    data.putInt("SpawnZ", spawnZ);
    data.putInt("GameType", gameType);
    data.putInt("Difficulty", difficulty);
    data.putLong("RandomSeed", seed);
    data.putInt("StorageVersion", 10);
    data.putInt("NetworkVersion", 2168);
    data.putByte("commandsEnabled", 1);
    data.putInt("limitedWorldOriginX", spawnX);
    data.putInt("limitedWorldOriginY", spawnY);
    data.putInt("limitedWorldOriginZ", spawnZ);
    data.putLong("LastPlayed", 0);

    BinaryStream body;
    NbtIo::writeTag(body, data, NbtVariant::LittleEndian);

    const std::string payload = body.getBuffer();

    BinaryStream file;
    file.putLInt(10);
    file.putLInt((uint32_t) payload.size());
    file.put(payload);

    std::ofstream out((root / "level.dat").string(), std::ios::binary);
    if (!out.is_open()) {
        LOG_WARN(LogAreaID::Server, "Could not write level.dat for %s", levelName.c_str());
        return;
    }

    const std::string bytes = file.getBuffer();
    out.write(bytes.data(), (std::streamsize) bytes.size());
}
