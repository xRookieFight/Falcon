#include "Network/BlockActionHandler.h"

#include "Block/BlockData.h"
#include "Core/Debug/BedrockLog.h"
#include "Actor/ServerPlayer.h"
#include "Network/BreakDebug.h"
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

#include <cmath>
#include <cstdlib>
#include <sstream>
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
            BreakDebug::log("calculateBreakSeconds: missing BlockData, fallback=0.05", &player);
            return 0.05;
        }

        if (blockData->mHardness < 0.0f) {
            std::ostringstream details;
            details << "block=" << blockData->mIdentifier << " hardness=" << blockData->mHardness
                    << " result=unbreakable";
            BreakDebug::log("calculateBreakSeconds", &player, details.str());
            return -1.0;
        }

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

        std::ostringstream details;
        details << "block=" << blockData->mIdentifier << " hardness=" << blockData->mHardness
                << " toolType=" << (int) blockData->mToolType << " toolTier=" << (int) blockData->mToolTier
                << " handHarvest=" << (blockData->mHandHarvest ? 1 : 0) << " matchesTool=" << (matchesTool ? 1 : 0)
                << " canHarvest=" << (canHarvest ? 1 : 0) << " speedMultiplier=" << speedMultiplier
                << " efficiency=" << efficiencyLevel << " haste=" << (haste == nullptr ? 0 : haste->level())
                << " miningFatigue=" << (fatigue == nullptr ? 0 : fatigue->level())
                << " onGround=" << (player.isOnGround() ? 1 : 0) << " flying=" << (player.isFlying() ? 1 : 0)
                << " inWater=" << (inWater ? 1 : 0) << " aquaAffinity=" << (hasAquaAffinity ? 1 : 0)
                << " resultSeconds=" << seconds;
        BreakDebug::log("calculateBreakSeconds", &player, details.str());
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

    int recipients = 0;
    for (auto &entry: owner.getPlayers()) {
        if (entry.second.getSentChunks().count(key) != 0) {
            owner.getNetworkHandler().send(entry.first, packet, owner.getCodecContext());
            ++recipients;
        }
    }

    std::ostringstream details;
    details << "packet=" << packet.getName() << " position=(" << position.x << ',' << position.y << ',' << position.z
            << ") chunk=(" << chunkX << ',' << chunkZ << ") key=" << key << " recipients=" << recipients;
    BreakDebug::log("broadcastToViewers", nullptr, details.str());
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
    if (distanceSquared > maxDistance * maxDistance) {
        std::ostringstream details;
        details << "block=(" << position.x << ',' << position.y << ',' << position.z << ") distanceSquared="
                << distanceSquared << " maxDistanceSquared=" << maxDistance * maxDistance << " result=too_far";
        BreakDebug::log("canInteractWithBlock", &player, details.str());
        return false;
    }

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
    std::ostringstream details;
    details << "block=(" << position.x << ',' << position.y << ',' << position.z << ") distanceSquared="
            << distanceSquared << " maxDistance=" << maxDistance << " yaw=" << player.getRotation().y
            << " direction=(" << directionX << ',' << directionZ << ") dotDelta=" << (dot1 - dot)
            << " reachMaxDiff=" << REACH_MAX_DIFF << " result=" << (result ? "allowed" : "bad_aim");
    BreakDebug::log("canInteractWithBlock", &player, details.str());
    return result;
}

void BlockActionHandler::breakBlock(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3i &position) {
    Level &level = owner.getLevel();

    {
        std::ostringstream details;
        details << "position=(" << position.x << ',' << position.y << ',' << position.z << ") phase=begin";
        BreakDebug::log("breakBlock", &player, details.str());
    }

    const BlockState brokenState = level.getChunk(position.x >> 4, position.z >> 4)
                                         .getBlock(position.x & 15, position.y, position.z & 15);

    if (brokenState.mName == "minecraft:air") {
        BreakDebug::log("breakBlock: target is air, no mutation", &player);
        return;
    }

    const int32_t brokenHash = BlockStateHasher::hash(brokenState.mName, brokenState.mStates);
    const BlockData *brokenData = BlockDataTable::find(brokenState.mName.c_str());

    {
        std::ostringstream details;
        details << "position=(" << position.x << ',' << position.y << ',' << position.z << ") block="
                << brokenState.mName << " hash=" << brokenHash << " blockData=" << (brokenData == nullptr ? "null" : "ok")
                << " gamemode=" << player.getGameType();
        BreakDebug::log("breakBlock: resolved target", &player, details.str());
    }

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
            {
                std::ostringstream details;
                details << "dropCandidate=" << dropIdentifier << " count=" << dropCount << " silkTouch="
                        << (silkTouch ? 1 : 0) << " fortune=" << fortuneLevel << " parsed=" << (parsed ? 1 : 0);
                BreakDebug::log("breakBlock: drop resolution", &player, details.str());
            }
            if (parsed) {
                ItemStack drop;
                drop.mDefinition = owner.getItemDefinitions().getDefinition(parsedItem.getIdentifier());
                drop.mBlockDefinition = owner.getBlockDefinitions().getDefinition(parsedItem.getIdentifier());
                drop.mCount = dropCount;

                const Vector3f dropPosition((float) position.x + 0.5f, (float) position.y + 0.5f,
                                            (float) position.z + 0.5f);
                owner.dropItem(dropPosition, drop, randomDropMotion(), ItemActor::DEFAULT_PICKUP_DELAY);
                BreakDebug::log("breakBlock: drop spawned", &player,
                                "identifier=" + parsedItem.getIdentifier() + " count=" + std::to_string(dropCount));
            }
        } else {
            std::ostringstream details;
            details << "dropCandidate=" << (dropIdentifier.empty() ? "<none>" : dropIdentifier)
                    << " count=" << dropCount << " reason=no_drop";
            BreakDebug::log("breakBlock: no drop", &player, details.str());
        }
    } else {
        BreakDebug::log("breakBlock: creative or missing BlockData, drops skipped", &player);
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

    BreakDebug::log("breakBlock: block set to air and packets sent", &player,
                    "position=(" + std::to_string(position.x) + "," + std::to_string(position.y) + "," +
                            std::to_string(position.z) + ") airHash=" + std::to_string(airHash));

    LOG_INFO(LogAreaID::Server, "%s broke block at %d %d %d", player.getName().c_str(), position.x, position.y,
             position.z);
}

void BlockActionHandler::startBreakingBlock(ServerNetworkHandler &owner, ServerPlayer &player,
                                            const Vector3i &position, int32_t face) {
    {
        std::ostringstream details;
        details << "requestedPosition=(" << position.x << ',' << position.y << ',' << position.z << ") face=" << face;
        BreakDebug::log("startBreakingBlock: received", &player, details.str());
    }

    if (player.isBreakingBlock() && player.getBreakingBlockPosition() == position) {
        BreakDebug::log("startBreakingBlock: duplicate target ignored", &player);
        return;
    }

    if (player.isBreakingBlock()) {
        BreakDebug::log("startBreakingBlock: stopping previous target", &player);
        stopBreakingBlock(owner, player);
    }

    Level &level = owner.getLevel();
    const BlockState &state = level.getChunk(position.x >> 4, position.z >> 4)
                                    .getBlock(position.x & 15, position.y, position.z & 15);

    if (state.mName == "minecraft:air") {
        BreakDebug::log("startBreakingBlock: target is air", &player);
        return;
    }

    const BlockData *blockData = BlockDataTable::find(state.mName.c_str());
    const double seconds = calculateBreakSeconds(player, blockData);

    if (seconds < 0.0) {
        BreakDebug::log("startBreakingBlock: target is unbreakable", &player,
                        "block=" + state.mName);
        return;
    }

    player.startBreakingBlock(position, face);

    const int ticks = calculateBreakTicks(seconds);

    LevelEventPacket start;
    start.mEventId = LevelEventPacket::Event::BlockStartBreak;
    start.mPosition = Vector3f((float) position.x + 0.5f, (float) position.y + 0.5f, (float) position.z + 0.5f);
    start.mData = 65535 / ticks;

    broadcastToViewers(owner, start.mPosition, start);

    std::ostringstream details;
    details << "position=(" << position.x << ',' << position.y << ',' << position.z << ") block=" << state.mName
            << " face=" << face << " seconds=" << seconds << " ticks=" << ticks << " startData=" << start.mData;
    BreakDebug::log("startBreakingBlock: state started", &player, details.str());
}

void BlockActionHandler::continueBreakingBlock(ServerNetworkHandler &owner, ServerPlayer &player) {
    if (!player.isBreakingBlock()) {
        BreakDebug::log("continueBreakingBlock: no active break", &player);
        return;
    }

    const Vector3i position = player.getBreakingBlockPosition();
    Level &level = owner.getLevel();
    const BlockState &state = level.getChunk(position.x >> 4, position.z >> 4)
                                    .getBlock(position.x & 15, position.y, position.z & 15);

    if (state.mName == "minecraft:air") {
        BreakDebug::log("continueBreakingBlock: target became air", &player);
        stopBreakingBlock(owner, player);
        return;
    }

    const BlockData *blockData = BlockDataTable::find(state.mName.c_str());
    const double seconds = calculateBreakSeconds(player, blockData);

    if (seconds < 0.0) {
        BreakDebug::log("continueBreakingBlock: target became unbreakable", &player,
                        "block=" + state.mName);
        stopBreakingBlock(owner, player);
        return;
    }

    const int ticks = calculateBreakTicks(seconds);
    const double previousProgress = player.getBreakProgress();
    const double progressDelta = 1.0 / (double) ticks;
    player.addBreakProgress(progressDelta);

    std::ostringstream details;
    details << "position=(" << position.x << ',' << position.y << ',' << position.z << ") block=" << state.mName
            << " seconds=" << seconds << " ticks=" << ticks << " previousProgress=" << previousProgress
            << " delta=" << progressDelta << " newProgress=" << player.getBreakProgress();
    BreakDebug::log("continueBreakingBlock: progress", &player, details.str());

    if (player.getBreakProgress() >= BREAK_PROGRESS_COMPLETE) {
        BreakDebug::log("continueBreakingBlock: completion threshold reached", &player,
                        "threshold=" + std::to_string(BREAK_PROGRESS_COMPLETE));
        completeBreakingBlock(owner, player, position);
    }
}

void BlockActionHandler::completeBreakingBlock(ServerNetworkHandler &owner, ServerPlayer &player,
                                               const Vector3i &position) {
    const int32_t face = player.getBreakingFace();
    std::ostringstream details;
    details << "requestedPosition=(" << position.x << ',' << position.y << ',' << position.z << ") face=" << face;
    BreakDebug::log("completeBreakingBlock: requested", &player, details.str());

    stopBreakingBlock(owner, player);

    if (!canInteractWithBlock(player, position)) {
        BreakDebug::log("completeBreakingBlock: rejected by interaction check", &player);
        return;
    }

    BreakDebug::log("completeBreakingBlock: interaction accepted", &player);
    breakBlock(owner, player, position);
}

void BlockActionHandler::sendBreakingFx(ServerNetworkHandler &owner, ServerPlayer &player) {
    const Vector3i position = player.getBreakingBlockPosition();
    Level &level = owner.getLevel();
    const BlockState &state = level.getChunk(position.x >> 4, position.z >> 4)
                                    .getBlock(position.x & 15, position.y, position.z & 15);

    if (state.mName == "minecraft:air") {
        BreakDebug::log("sendBreakingFx: target is air", &player);
        return;
    }

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

    std::ostringstream details;
    details << "position=(" << position.x << ',' << position.y << ',' << position.z << ") block=" << state.mName
            << " hash=" << blockHash << " face=" << player.getBreakingFace();
    BreakDebug::log("sendBreakingFx: hit and punch sent", &player, details.str());
}

void BlockActionHandler::stopBreakingBlock(ServerNetworkHandler &owner, ServerPlayer &player) {
    if (!player.isBreakingBlock()) {
        BreakDebug::log("stopBreakingBlock: no active break", &player);
        return;
    }

    const Vector3i position = player.getBreakingBlockPosition();
    const double progress = player.getBreakProgress();
    player.stopBreakingBlock();

    LevelEventPacket stop;
    stop.mEventId = LevelEventPacket::Event::BlockStopBreak;
    stop.mPosition = Vector3f((float) position.x + 0.5f, (float) position.y + 0.5f, (float) position.z + 0.5f);
    stop.mData = 0;

    broadcastToViewers(owner, stop.mPosition, stop);

    std::ostringstream details;
    details << "position=(" << position.x << ',' << position.y << ',' << position.z << ") progressBefore=" << progress;
    BreakDebug::log("stopBreakingBlock: state stopped", &player, details.str());
}

void BlockActionHandler::placeBlock(ServerNetworkHandler &owner, ServerPlayer &player,
                                    const ItemUseTransaction &transaction) {
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

    LOG_INFO(LogAreaID::Server, "%s placed %s at %d %d %d", player.getName().c_str(),
             definition.getIdentifier().c_str(), target.x, target.y, target.z);
}
