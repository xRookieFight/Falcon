#include "Network/BlockActionHandler.h"

#include "Block/BlockData.h"
#include "Actor/ServerPlayer.h"
#include "Item/ItemData.h"
#include "Item/ItemEnchantments.h"
#include "Item/ItemNetworkIdTable.h"
#include "Item/StringToItemParser.h"
#include "Level/Level.h"
#include "Network/NetworkHandler.h"
#include "Network/ServerNetworkHandler.h"
#include "Protocol/BlockStateHasher.h"
#include "Protocol/Packets/LevelEventPacket.h"
#include "Protocol/Packets/LevelSoundEventPacket.h"
#include "Protocol/Packets/UpdateBlockPacket.h"
#include "Protocol/Types/ItemUseTransaction.h"
#include "Protocol/Types/StartGameTypes.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

namespace {
    const float PLAYER_BASE_OFFSET = 1.62f;
    const double BREAK_PROGRESS_COMPLETE = 0.99;
    const float REACH_MAX_DIFF = 6.0f;

    float randomUnitFloat() {
        return (float) (rand() % 1000) / 1000.0f;
    }

    Vector3f randomDropMotion() {
        return Vector3f(randomUnitFloat() * 0.2f - 0.1f, 0.2f, randomUnitFloat() * 0.2f - 0.1f);
    }

    float toolSpeedMultiplier(BlockToolTier tier) {
        switch (tier) {
            case BlockToolTier::Wooden:
                return 2.0f;
            case BlockToolTier::Stone:
                return 4.0f;
            case BlockToolTier::Iron:
                return 6.0f;
            case BlockToolTier::Diamond:
                return 8.0f;
            case BlockToolTier::Netherite:
                return 9.0f;
            case BlockToolTier::Gold:
                return 12.0f;
            default:
                return 1.0f;
        }
    }

    double calculateBreakSeconds(ServerPlayer &player, const BlockData *blockData) {
        if (blockData == nullptr) {
            return 0.05;
        }

        if (blockData->mHardness < 0.0f)
            return -1.0;

        const double blockHardness = (double) blockData->mHardness;
        const ItemStack &heldItem = player.getInventory().getItemInHand();
        const ItemData *itemData = heldItem.isAir() || heldItem.mDefinition == nullptr
                                  ? nullptr
                                  : ItemDataTable::find(heldItem.mDefinition->getIdentifier());

        const bool matchesTool = itemData != nullptr && blockData->mToolType != BlockToolType::None &&
                                (int32_t) itemData->mToolType == (int32_t) blockData->mToolType &&
                                itemData->mToolTier >= (int32_t) blockData->mToolTier;

        const bool canHarvest = blockData->mToolType == BlockToolType::None || blockData->mHandHarvest ||
                                matchesTool;

        double seconds = canHarvest ? blockHardness * 1.5 : blockHardness * 5.0;

        float speedMultiplier = 1.0f;
        const bool correctTool = matchesTool;

        if (correctTool)
            speedMultiplier = toolSpeedMultiplier((BlockToolTier) (itemData == nullptr ? 0 : itemData->mToolTier));

        const int32_t efficiencyLevel = ItemEnchantments::getLevel(heldItem, EnchantmentIds::EFFICIENCY);
        if (correctTool && canHarvest && efficiencyLevel > 0)
            speedMultiplier += (float) (efficiencyLevel * efficiencyLevel + 1);

        const MobEffectInstance *haste = player.getEffect(MobEffectId::Haste);
        const MobEffectInstance *fatigue = player.getEffect(MobEffectId::MiningFatigue);
        if (haste)
            speedMultiplier *= 1.0f + 0.2f * (float) haste->level();
        if (fatigue)
            speedMultiplier *= std::pow(0.3f, (float) fatigue->level());

        seconds /= (double) speedMultiplier;

        if (!player.isOnGround() && !player.isFlying())
            seconds *= 5.0;

        const bool inWater = player.getFlags().get(ActorFlag::Swimming);
        const bool hasAquaAffinity =
                ItemEnchantments::getLevel(player.getInventory().getArmor(PlayerInventory::ARMOR_HEAD),
                                           EnchantmentIds::AQUA_AFFINITY) > 0;
        if (inWater && !hasAquaAffinity)
            seconds *= 5.0;

        return seconds;
    }

    int calculateBreakTicks(double seconds) {
        if (seconds <= 0.0)
            return 1;

        const int ticks = (int) std::ceil(seconds * 20.0);
        return ticks < 1 ? 1 : ticks;
    }
}

void BlockActionHandler::broadcastToViewers(ServerNetworkHandler &owner, const Vector3f &position,
                                            const Packet &packet) {
    const int32_t chunkX = (int32_t) std::floor(position.x) >> 4;
    const int32_t chunkZ = (int32_t) std::floor(position.z) >> 4;
    const int64_t key = ((int64_t) chunkX << 32) | (uint32_t) chunkZ;

    for (auto &entry: owner.getPlayers()) {
        if (entry.second.getSentChunks().count(key) != 0) {
            owner.getNetworkHandler().send(entry.first, packet, owner.getCodecContext());
        }
    }
}

bool BlockActionHandler::canInteractWithBlock(ServerPlayer &player, const Vector3i &position) {
    const float maxDistance = player.getGameType() == (int32_t) GameType::Creative ? 13.0f : 7.0f;

    const Vector3f feet = player.getPosition();
    const Vector3f eyePosition(feet.x, feet.y + PLAYER_BASE_OFFSET, feet.z);
    const Vector3f target((float) position.x + 0.5f, (float) position.y + 0.5f, (float) position.z + 0.5f);

    const float dx = target.x - eyePosition.x;
    const float dy = target.y - eyePosition.y;
    const float dz = target.z - eyePosition.z;

    const float distanceSquared = dx * dx + dy * dy + dz * dz;
    if (distanceSquared > maxDistance * maxDistance)
        return false;

    const float yawRad = player.getRotation().y * 3.14159265f / 180.0f;
    const float halfPi = 1.57079633f;
    float directionX = -std::cos(yawRad - halfPi);
    float directionZ = -std::sin(yawRad - halfPi);

    const float length = std::sqrt(directionX * directionX + directionZ * directionZ);
    if (length > 0.0f) {
        directionX /= length;
        directionZ /= length;
    }

    const float dot = directionX * eyePosition.x + directionZ * eyePosition.z;
    const float dot1 = directionX * target.x + directionZ * target.z;

    const bool result = (dot1 - dot) >= -REACH_MAX_DIFF;
    return result;
}

void BlockActionHandler::breakBlock(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3i &position) {
    Level &level = owner.getLevel();

    const BlockState brokenState = level.getChunk(position.x >> 4, position.z >> 4)
                                         .getBlock(position.x & 15, position.y, position.z & 15);

    if (brokenState.mName == "minecraft:air") {
        return;
    }

    const int32_t brokenHash = BlockStateHasher::hash(brokenState.mName, brokenState.mStates);
    const BlockData *brokenData = BlockDataTable::find(brokenState.mName.c_str());

    const int32_t airHash = level.getAirHash();
    level.setBlockState(position.x, position.y, position.z, BlockState("minecraft:air"));

    if (brokenData != nullptr && player.getGameType() != (int32_t) GameType::Creative) {
        const ItemStack &heldItem = player.getInventory().getItemInHand();
        const bool silkTouch = ItemEnchantments::getLevel(heldItem, EnchantmentIds::SILK_TOUCH) > 0;
        const int32_t fortuneLevel = ItemEnchantments::getLevel(heldItem, EnchantmentIds::FORTUNE);

        std::string dropIdentifier;
        int32_t dropCount = 0;

        if (silkTouch && brokenData->mSilkTouch) {
            dropIdentifier = brokenState.mName;
            dropCount = 1;
        } else if (brokenData->mDropKind == BlockDropKind::Self) {
            dropIdentifier = brokenState.mName;
            dropCount = brokenData->mDropMin;
        } else if (brokenData->mDropKind == BlockDropKind::Other && brokenData->mDropIdentifier != nullptr) {
            dropIdentifier = brokenData->mDropIdentifier;
            const int32_t range = (int32_t) brokenData->mDropMax - (int32_t) brokenData->mDropMin + 1;
            dropCount = (int32_t) brokenData->mDropMin + (range > 0 ? rand() % range : 0);

            if (fortuneLevel > 0)
                dropCount += rand() % (fortuneLevel + 1);
        }

        if (!dropIdentifier.empty() && dropCount > 0) {
            Item parsedItem;
            const bool parsed = StringToItemParser::getInstance().parse(dropIdentifier, parsedItem);
            if (parsed) {
                ItemStack drop;
                drop.mDefinition = owner.getItemDefinitions().getDefinition(parsedItem.getIdentifier());
                drop.mBlockDefinition = owner.getBlockDefinitions().getDefinition(parsedItem.getIdentifier());
                drop.mCount = dropCount;

                const Vector3f dropPosition((float) position.x + 0.5f, (float) position.y + 0.5f,
                                            (float) position.z + 0.5f);
                owner.dropItem(dropPosition, drop, randomDropMotion(), ItemActor::DEFAULT_PICKUP_DELAY);
            }
        }
    }

    UpdateBlockPacket update;
    update.mBlockPosition = position;
    update.mRuntimeId = (uint32_t) airHash;
    update.mFlags = UpdateBlockPacket::Flag::All;
    update.mDataLayer = 0;

    LevelEventPacket destroy;
    destroy.mEventId = LevelEventPacket::Event::ParticleDestroy;
    destroy.mPosition = Vector3f((float) position.x + 0.5f, (float) position.y + 0.5f, (float) position.z + 0.5f);
    destroy.mData = brokenHash;

    broadcastToViewers(owner, destroy.mPosition, update);
    broadcastToViewers(owner, destroy.mPosition, destroy);

    player.exhaust(0.005f);

}

void BlockActionHandler::startBreakingBlock(ServerNetworkHandler &owner, ServerPlayer &player,
                                            const Vector3i &position, int32_t face) {
    if (player.isBreakingBlock() && player.getBreakingBlockPosition() == position)
        return;

    if (player.isBreakingBlock())
        stopBreakingBlock(owner, player);

    Level &level = owner.getLevel();
    const BlockState &state = level.getChunk(position.x >> 4, position.z >> 4)
                                    .getBlock(position.x & 15, position.y, position.z & 15);

    if (state.mName == "minecraft:air") {
        return;
    }

    const BlockData *blockData = BlockDataTable::find(state.mName.c_str());
    const double seconds = calculateBreakSeconds(player, blockData);

    if (seconds < 0.0) {
        return;
    }

    player.startBreakingBlock(position, face);

    const int ticks = calculateBreakTicks(seconds);

    LevelEventPacket start;
    start.mEventId = LevelEventPacket::Event::BlockStartBreak;
    start.mPosition = Vector3f((float) position.x + 0.5f, (float) position.y + 0.5f, (float) position.z + 0.5f);
    start.mData = 65535 / ticks;

    broadcastToViewers(owner, start.mPosition, start);
}

void BlockActionHandler::continueBreakingBlock(ServerNetworkHandler &owner, ServerPlayer &player) {
    if (!player.isBreakingBlock())
        return;

    const Vector3i position = player.getBreakingBlockPosition();
    Level &level = owner.getLevel();
    const BlockState &state = level.getChunk(position.x >> 4, position.z >> 4)
                                    .getBlock(position.x & 15, position.y, position.z & 15);

    if (state.mName == "minecraft:air") {
        stopBreakingBlock(owner, player);
        return;
    }

    const BlockData *blockData = BlockDataTable::find(state.mName.c_str());
    const double seconds = calculateBreakSeconds(player, blockData);

    if (seconds < 0.0) {
        stopBreakingBlock(owner, player);
        return;
    }

    const int ticks = calculateBreakTicks(seconds);
    const double progressDelta = 1.0 / (double) ticks;
    player.addBreakProgress(progressDelta);

    LevelEventPacket update;
    update.mEventId = LevelEventPacket::Event::BlockUpdateBreak;
    update.mPosition = Vector3f((float) position.x + 0.5f, (float) position.y + 0.5f,
                                (float) position.z + 0.5f);
    update.mData = (int32_t) std::clamp(65535.0 * progressDelta, 1.0, 65535.0);
    broadcastToViewers(owner, update.mPosition, update);

    if (player.getBreakProgress() >= BREAK_PROGRESS_COMPLETE) {
        completeBreakingBlock(owner, player, position);
    }
}

void BlockActionHandler::completeBreakingBlock(ServerNetworkHandler &owner, ServerPlayer &player,
                                               const Vector3i &position) {
    stopBreakingBlock(owner, player);

    if (!canInteractWithBlock(player, position))
        return;

    breakBlock(owner, player, position);
}

void BlockActionHandler::sendBreakingFx(ServerNetworkHandler &owner, ServerPlayer &player) {
    const Vector3i position = player.getBreakingBlockPosition();
    Level &level = owner.getLevel();
    const BlockState &state = level.getChunk(position.x >> 4, position.z >> 4)
                                    .getBlock(position.x & 15, position.y, position.z & 15);

    if (state.mName == "minecraft:air")
        return;

    const int32_t blockHash = BlockStateHasher::hash(state.mName, state.mStates);
    const Vector3f center((float) position.x + 0.5f, (float) position.y + 0.5f, (float) position.z + 0.5f);

    LevelEventPacket punch;
    punch.mEventId = LevelEventPacket::Event::ParticlePunchBlock;
    punch.mPosition = center;
    punch.mData = blockHash | (player.getBreakingFace() << 24);
    broadcastToViewers(owner, center, punch);

    LevelSoundEventPacket hit;
    hit.mSound = "hit";
    hit.mPosition = center;
    hit.mExtraData = blockHash;
    hit.mEntityType = "";
    hit.mActorUniqueId = -1;
    broadcastToViewers(owner, center, hit);

}

void BlockActionHandler::stopBreakingBlock(ServerNetworkHandler &owner, ServerPlayer &player) {
    if (!player.isBreakingBlock())
        return;

    const Vector3i position = player.getBreakingBlockPosition();
    player.stopBreakingBlock();

    LevelEventPacket stop;
    stop.mEventId = LevelEventPacket::Event::BlockStopBreak;
    stop.mPosition = Vector3f((float) position.x + 0.5f, (float) position.y + 0.5f, (float) position.z + 0.5f);
    stop.mData = 0;

    broadcastToViewers(owner, stop.mPosition, stop);

}

void BlockActionHandler::placeBlock(ServerNetworkHandler &owner, ServerPlayer &player,
                                    const ItemUseTransaction &transaction) {
    (void) player;

    if (transaction.mItemInHand.mBlockDefinition == nullptr)
        return;

    static const int offsets[6][3] = {
            {0,  -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}
    };

    const int face = transaction.mBlockFace >= 0 && transaction.mBlockFace < 6 ? transaction.mBlockFace : 1;
    const Vector3i target(transaction.mBlockPosition.x + offsets[face][0],
                          transaction.mBlockPosition.y + offsets[face][1],
                          transaction.mBlockPosition.z + offsets[face][2]);

    const BlockDefinition &definition = *transaction.mItemInHand.mBlockDefinition;
    const int32_t blockHash = BlockStateHasher::hash(definition.getIdentifier(), definition.getState());

    Level &level = owner.getLevel();
    level.setBlockState(target.x, target.y, target.z, BlockState(definition.getIdentifier(), definition.getState()));

    UpdateBlockPacket update;
    update.mBlockPosition = target;
    update.mRuntimeId = (uint32_t) blockHash;
    update.mFlags = UpdateBlockPacket::Flag::All;
    update.mDataLayer = 0;

    const Vector3f targetCenter((float) target.x + 0.5f, (float) target.y + 0.5f, (float) target.z + 0.5f);
    broadcastToViewers(owner, targetCenter, update);

    LevelSoundEventPacket placeSound;
    placeSound.mSound = "place";
    placeSound.mPosition = targetCenter;
    placeSound.mExtraData = (int32_t) blockHash;
    placeSound.mEntityType = "";
    placeSound.mActorUniqueId = -1;
    broadcastToViewers(owner, targetCenter, placeSound);

}
