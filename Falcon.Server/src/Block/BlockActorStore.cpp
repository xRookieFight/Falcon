#include "Block/BlockActorStore.h"

#include "Block/Actor/ChestBlockActor.h"
#include "Block/Actor/CommandBlockActor.h"
#include "Block/Actor/FurnaceBlockActor.h"
#include "Block/Actor/PistonArmBlockActor.h"

#include <utility>

BlockActorStore &BlockActorStore::getInstance() {
    static BlockActorStore instance;
    return instance;
}

int64_t BlockActorStore::packPosition(const Vector3i &position) {
    return ((int64_t) position.x & 0x3ffffff) << 38 | ((int64_t) position.y & 0xfff) << 26 |
           ((int64_t) position.z & 0x3ffffff);
}

BlockActor *BlockActorStore::find(const Vector3i &position) {
    auto it = mBlockActors.find(packPosition(position));
    return it == mBlockActors.end() ? nullptr : it->second.get();
}

void BlockActorStore::insert(std::unique_ptr<BlockActor> blockActor) {
    if (blockActor == nullptr)
        return;

    mBlockActors[packPosition(blockActor->getPosition())] = std::move(blockActor);
}

void BlockActorStore::remove(const Vector3i &position) {
    mBlockActors.erase(packPosition(position));
}

std::vector<Tag> BlockActorStore::saveChunk(int32_t chunkX, int32_t chunkZ) const {
    std::vector<Tag> saved;

    for (const auto &entry: mBlockActors) {
        const Vector3i &position = entry.second->getPosition();
        if ((position.x >> 4) != chunkX || (position.z >> 4) != chunkZ)
            continue;

        saved.push_back(entry.second->saveWithPosition());
    }

    return saved;
}

void BlockActorStore::unloadChunk(int32_t chunkX, int32_t chunkZ) {
    for (auto it = mBlockActors.begin(); it != mBlockActors.end();) {
        const Vector3i &position = it->second->getPosition();
        if ((position.x >> 4) == chunkX && (position.z >> 4) == chunkZ)
            it = mBlockActors.erase(it);
        else
            ++it;
    }
}

void BlockActorStore::loadChunk(const std::vector<Tag> &blockActors, const PacketCodecContext &context) {
    for (const Tag &data: blockActors) {
        if (data.getType() != Tag::Type::Compound)
            continue;

        std::unique_ptr<BlockActor> blockActor = create(data.getString("id"));
        if (blockActor == nullptr)
            continue;

        blockActor->setPosition(Vector3i(data.getInt("x"), data.getInt("y"), data.getInt("z")));
        blockActor->loadNbt(data, context);
        insert(std::move(blockActor));
    }
}

std::unique_ptr<BlockActor> BlockActorStore::create(const std::string &blockActorId) {
    if (blockActorId == ChestBlockActor::BLOCK_ACTOR_ID)
        return std::unique_ptr<BlockActor>(new ChestBlockActor());
    if (blockActorId == FurnaceBlockActor::BLOCK_ACTOR_ID)
        return std::unique_ptr<BlockActor>(new FurnaceBlockActor());
    if (blockActorId == PistonArmBlockActor::BLOCK_ACTOR_ID)
        return std::unique_ptr<BlockActor>(new PistonArmBlockActor());
    if (blockActorId == CommandBlockActor::BLOCK_ACTOR_ID)
        return std::unique_ptr<BlockActor>(new CommandBlockActor());

    return nullptr;
}
