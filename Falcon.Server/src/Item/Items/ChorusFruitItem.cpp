#include "Item/Items/ChorusFruitItem.h"

#include "Actor/ServerPlayer.h"
#include "Block/BlockData.h"
#include "Block/Blocks/LiquidBlock.h"
#include "Block/Systems/LiquidBlocksFetch.h"
#include "Level/LevelChunk.h"
#include "Level/Level.h"
#include "Network/Handler/BlockActionHandler.h"
#include "Network/Handler/ServerNetworkHandler.h"
#include "Protocol/Packets/LevelSoundEventPacket.h"
#include "Protocol/Types/ItemStack.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <string>

namespace {
    const char *CHORUS_FRUIT_IDENTIFIER = "minecraft:chorus_fruit";
    const int TELEPORT_RANGE = 8;
    const int TELEPORT_ATTEMPTS = 128;

    std::mt19937 &randomGenerator() {
        static std::mt19937 generator(std::random_device{}());
        return generator;
    }
}

bool ChorusFruitItem::isChorusFruit(const ItemStack &item) {
    return item.mDefinition != nullptr && item.mDefinition->getIdentifier() == CHORUS_FRUIT_IDENTIFIER;
}

bool ChorusFruitItem::canConsume(ServerNetworkHandler &owner, ServerPlayer &player) {
    if (!isChorusFruit(player.getInventory().getItemInHand()))
        return false;

    return !LiquidBlocksFetch::at(owner.getLevel(), player.getPosition()).water;
}

bool ChorusFruitItem::isSolid(ServerNetworkHandler &owner, int x, int y, int z) {
    const int minWorldY = LevelChunk::LOWEST_SUB_CHUNK_Y * 16;
    const int maxWorldY = minWorldY + LevelChunk::SUB_CHUNK_COUNT * 16 - 1;
    if (y < minWorldY || y > maxWorldY)
        return true;

    const BlockState state = owner.getLevel().getBlockState(x, y, z);
    const BlockData *data = BlockDataTable::find(state.mName.c_str());
    return data != nullptr && data->mSolid;
}

bool ChorusFruitItem::isLiquid(ServerNetworkHandler &owner, int x, int y, int z) {
    const int minWorldY = LevelChunk::LOWEST_SUB_CHUNK_Y * 16;
    const int maxWorldY = minWorldY + LevelChunk::SUB_CHUNK_COUNT * 16 - 1;
    if (y < minWorldY || y > maxWorldY)
        return true;

    return LiquidBlock(owner.getLevel().getBlockState(x, y, z)).isLiquid();
}

bool ChorusFruitItem::findTeleportPosition(ServerNetworkHandler &owner, ServerPlayer &player,
                                            Vector3f &destination) {
    const Vector3f origin = player.getPosition();
    const int minWorldY = LevelChunk::LOWEST_SUB_CHUNK_Y * 16;
    const int maxWorldY = minWorldY + LevelChunk::SUB_CHUNK_COUNT * 16 - 1;
    const int minX = (int) std::floor(origin.x) - TELEPORT_RANGE;
    const int minY = std::max(minWorldY, (int) std::floor(origin.y) - TELEPORT_RANGE);
    const int minZ = (int) std::floor(origin.z) - TELEPORT_RANGE;
    const int maxX = minX + TELEPORT_RANGE * 2;
    const int maxY = std::min(maxWorldY - 2, (int) std::floor(origin.y) + TELEPORT_RANGE);
    const int maxZ = minZ + TELEPORT_RANGE * 2;

    if (minY > maxY)
        return false;

    std::uniform_int_distribution<int> xDistribution(minX, maxX);
    std::uniform_int_distribution<int> yDistribution(minY, maxY);
    std::uniform_int_distribution<int> zDistribution(minZ, maxZ);

    for (int attempt = 0; attempt < TELEPORT_ATTEMPTS; ++attempt) {
        const int x = xDistribution(randomGenerator());
        int y = yDistribution(randomGenerator());
        const int z = zDistribution(randomGenerator());

        while (y >= minWorldY && !isSolid(owner, x, y + 1, z))
            --y;

        ++y;
        if (y < minWorldY || y + 2 > maxWorldY)
            continue;

        if (isSolid(owner, x, y + 1, z) || isLiquid(owner, x, y + 1, z)
            || isSolid(owner, x, y + 2, z) || isLiquid(owner, x, y + 2, z))
            continue;

        destination = Vector3f((float) x + 0.5f, (float) y + 1.0f, (float) z + 0.5f);
        return true;
    }

    return false;
}

void ChorusFruitItem::sendTeleportSound(ServerNetworkHandler &owner, ServerPlayer &player,
                                         const Vector3f &position) {
    LevelSoundEventPacket sound;
    sound.mSound = "teleport";
    sound.mPosition = position;
    sound.mExtraData = -1;
    sound.mActorType = player.getIdentifier();
    sound.mActorUniqueId = player.getUniqueId();
    sound.mIsBabyMob = false;
    sound.mDisableRelativeVolume = false;
    sound.mHasFirePosition = false;
    BlockActionHandler::broadcastToViewers(owner, position, sound);
}

bool ChorusFruitItem::onEaten(ServerNetworkHandler &owner, ServerPlayer &player) {
    Vector3f destination;
    if (!findTeleportPosition(owner, player, destination))
        return true;

    const Vector3f source = player.getPosition();
    sendTeleportSound(owner, player, source);
    player.teleport(owner, destination, MovePlayerTeleportationCause::ChorusFruit);
    sendTeleportSound(owner, player, destination);
    return true;
}
