#pragma once

#include "Block/BlockActor.h"
#include "Core/Math/Vector3i.h"
#include "Core/NBT/Tag.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class PacketCodecContext;

class BlockActorStore {
public:
    static BlockActorStore &getInstance();

    static int64_t packPosition(const Vector3i &position);

    BlockActor *find(const Vector3i &position);

    template <typename T>
    T *find(const Vector3i &position) {
        return dynamic_cast<T *>(find(position));
    }

    template <typename T>
    T &getOrCreate(const Vector3i &position) {
        T *existing = find<T>(position);
        if (existing != nullptr)
            return *existing;

        std::unique_ptr<T> created(new T());
        created->setPosition(position);
        T &reference = *created;
        insert(std::move(created));
        return reference;
    }

    void insert(std::unique_ptr<BlockActor> blockActor);

    void remove(const Vector3i &position);

    std::vector<Tag> saveChunk(int32_t chunkX, int32_t chunkZ) const;

    void unloadChunk(int32_t chunkX, int32_t chunkZ);

    void loadChunk(const std::vector<Tag> &blockActors, const PacketCodecContext &context);

    static std::unique_ptr<BlockActor> create(const std::string &blockActorId);

    template <typename T>
    std::vector<T *> findAll() {
        std::vector<T *> matches;
        for (auto &entry: mBlockActors) {
            T *typed = dynamic_cast<T *>(entry.second.get());
            if (typed != nullptr)
                matches.push_back(typed);
        }
        return matches;
    }

private:
    std::unordered_map<int64_t, std::unique_ptr<BlockActor>> mBlockActors;
};
