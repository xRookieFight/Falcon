#include "Network/BlockActionHandler.h"

#include "Block/BlockData.h"
#include "Actor/ServerPlayer.h"
#include "Item/ItemData.h"
#include "Item/ItemEnchantments.h"
#include "Item/Items/BucketItem.h"
#include "Item/StringToItemParser.h"
#include "Inventory/InventoryManager.h"
#include "Level/Level.h"
#include "Network/InventoryHandler.h"
#include "Network/NetworkHandler.h"
#include "Network/ServerNetworkHandler.h"
#include "Protocol/BlockStateHasher.h"
#include "Protocol/Packets/EntityEventPacket.h"
#include "Protocol/Packets/LevelEventPacket.h"
#include "Protocol/Packets/LevelSoundEventPacket.h"
#include "Protocol/Packets/UpdateBlockPacket.h"
#include "Protocol/Types/ItemUseTransaction.h"
#include "Protocol/Types/StartGameTypes.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <string>
#include <utility>

namespace {
    const float PLAYER_BASE_OFFSET = 1.62f;
    const double BREAK_SPEED_CHANGE_EPSILON = 0.0001;
    const double BREAK_PROGRESS_EPSILON = 0.000001;
    const float REACH_MAX_DIFF = 0.8660254f;

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
            speedMultiplier *= (1.0 + 0.2 * (double) haste->level())
                               * std::pow(1.2, (double) haste->level());
        if (fatigue)
            speedMultiplier *= std::pow(0.21, (double) fatigue->level());

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

    double calculateBreakProgressPerTick(ServerPlayer &player, const BlockData *blockData) {
        const double seconds = calculateBreakSeconds(player, blockData);
        if (seconds < 0.0)
            return 0.0;
        if (seconds <= 0.0)
            return 1.0;

        return 1.0 / (seconds * 20.0);
    }

    int32_t breakSpeedEventData(double speed) {
        return (int32_t) std::clamp(65535.0 * speed, 0.0, 65535.0);
    }

    int64_t expectedBreakTicks(ServerPlayer &player, const BlockData *blockData) {
        const double seconds = calculateBreakSeconds(player, blockData);
        if (seconds < 0.0)
            return std::numeric_limits<int64_t>::max();

        const double ticks = std::ceil(seconds * 20.0) - 1.0;
        return std::max<int64_t>(0, (int64_t) ticks);
    }

    bool hasReachedBreakTime(ServerNetworkHandler &owner, ServerPlayer &player,
                             const BlockData *blockData) {
        if (!player.isBreakingBlock())
            return false;

        const int64_t elapsedTicks = owner.getCurrentTick() - player.getBreakStartTick();
        return elapsedTicks >= expectedBreakTicks(player, blockData);
    }

    bool finiteClickPosition(const Vector3f &clickPosition) {
        return std::isfinite(clickPosition.x) && std::isfinite(clickPosition.y)
               && std::isfinite(clickPosition.z);
    }

    bool isReplaceable(const BlockState &state) {
        if (state.mName == "minecraft:air")
            return true;

        const BlockData *data = BlockDataTable::find(state.mName.c_str());
        return data != nullptr && !data->mSolid;
    }

    bool intersectsPlayer(const ServerPlayer &player, const Vector3i &position, const BlockData *blockData) {
        if (blockData == nullptr || !blockData->mSolid)
            return false;

        const Vector3f &playerPosition = player.getPosition();
        constexpr float halfWidth = 0.3f;
        constexpr float playerHeight = 1.8f;

        const float playerMinX = playerPosition.x - halfWidth;
        const float playerMaxX = playerPosition.x + halfWidth;
        const float playerMinY = playerPosition.y;
        const float playerMaxY = playerPosition.y + playerHeight;
        const float playerMinZ = playerPosition.z - halfWidth;
        const float playerMaxZ = playerPosition.z + halfWidth;

        return playerMinX < (float) position.x + 1.0f && playerMaxX > (float) position.x
               && playerMinY < (float) position.y + 1.0f && playerMaxY > (float) position.y
               && playerMinZ < (float) position.z + 1.0f && playerMaxZ > (float) position.z;
    }

    void sendCurrentBlockState(ServerNetworkHandler &owner, const Vector3i &position) {
        if (position.y < Chunk::MIN_Y || position.y > Chunk::MAX_Y)
            return;

        const BlockState state = owner.getLevel().getBlockState(position.x, position.y, position.z);

        UpdateBlockPacket update;
        update.mBlockPosition = position;
        update.mRuntimeId = (uint32_t) BlockStateHasher::hash(state.mName, state.mStates);
        update.mFlags = UpdateBlockPacket::Flag::All;
        update.mDataLayer = 0;
        BlockActionHandler::broadcastToViewers(owner,
                                               Vector3f((float) position.x + 0.5f,
                                                        (float) position.y + 0.5f,
                                                        (float) position.z + 0.5f),
                                               update);
    }

    void broadcastArmSwing(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3f &position) {
        EntityEventPacket swing;
        swing.mRuntimeActorId = player.getRuntimeId();
        swing.mEventId = (uint8_t) EntityEventType::ArmSwing;
        swing.mEventData = 0;
        swing.mHasFirePosition = false;
        BlockActionHandler::broadcastToViewers(owner, position, swing);
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
    const float pitchRad = player.getRotation().x * 3.14159265f / 180.0f;
    const float directionX = -std::sin(yawRad) * std::cos(pitchRad);
    const float directionY = -std::sin(pitchRad);
    const float directionZ = std::cos(yawRad) * std::cos(pitchRad);

    const float dot = directionX * eyePosition.x + directionY * eyePosition.y + directionZ * eyePosition.z;
    const float dot1 = directionX * target.x + directionY * target.y + directionZ * target.z;
    return (dot1 - dot) >= -REACH_MAX_DIFF;
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
    if (position.y < Chunk::MIN_Y || position.y > Chunk::MAX_Y)
        return;

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
    const bool creative = player.getGameType() == (int32_t) GameType::Creative;
    if (creative && blockData != nullptr && blockData->mHardness < 0.0f)
        return;

    const double breakSpeed = creative ? 1.0 : calculateBreakProgressPerTick(player, blockData);

    if (breakSpeed <= 0.0) {
        return;
    }

    player.startBreakingBlock(position, face, breakSpeed, owner.getCurrentTick());

    LevelEventPacket start;
    start.mEventId = LevelEventPacket::Event::BlockStartBreak;
    start.mPosition = Vector3f((float) position.x + 0.5f, (float) position.y + 0.5f, (float) position.z + 0.5f);
    start.mData = breakSpeedEventData(breakSpeed);

    broadcastToViewers(owner, start.mPosition, start);

    if (creative)
        completeBreakingBlock(owner, player, position);
}

void BlockActionHandler::continueBreakingBlock(ServerNetworkHandler &owner, ServerPlayer &player) {
    if (!player.isBreakingBlock())
        return;

    if (owner.getCurrentTick() <= player.getBreakStartTick())
        return;

    const Vector3i position = player.getBreakingBlockPosition();
    if (position.y < Chunk::MIN_Y || position.y > Chunk::MAX_Y) {
        stopBreakingBlock(owner, player);
        return;
    }

    Level &level = owner.getLevel();
    const BlockState &state = level.getChunk(position.x >> 4, position.z >> 4)
                                    .getBlock(position.x & 15, position.y, position.z & 15);

    if (state.mName == "minecraft:air") {
        stopBreakingBlock(owner, player);
        return;
    }

    const BlockData *blockData = BlockDataTable::find(state.mName.c_str());
    const double newBreakSpeed = calculateBreakProgressPerTick(player, blockData);

    if (newBreakSpeed <= 0.0) {
        stopBreakingBlock(owner, player);
        return;
    }

    if (std::fabs(newBreakSpeed - player.getBreakSpeed()) > BREAK_SPEED_CHANGE_EPSILON) {
        player.setBreakSpeed(newBreakSpeed);

        LevelEventPacket update;
        update.mEventId = LevelEventPacket::Event::BlockUpdateBreak;
        update.mPosition = Vector3f((float) position.x + 0.5f, (float) position.y + 0.5f,
                                    (float) position.z + 0.5f);
        update.mData = breakSpeedEventData(newBreakSpeed);
        broadcastToViewers(owner, update.mPosition, update);
    }

    player.addBreakProgress(player.getBreakSpeed());

    if (player.getBreakProgress() >= 1.0) {
        completeBreakingBlock(owner, player, position);
    }
}

void BlockActionHandler::completeBreakingBlock(ServerNetworkHandler &owner, ServerPlayer &player,
                                               const Vector3i &position) {
    if (position.y < Chunk::MIN_Y || position.y > Chunk::MAX_Y)
        return;

    if (!canInteractWithBlock(player, position))
        return;

    const bool creative = player.getGameType() == (int32_t) GameType::Creative;
    if (creative && !player.isBreakingBlock()) {
        startBreakingBlock(owner, player, position, 0);
        return;
    }

    if (!creative) {
        if (!player.isBreakingBlock() || player.getBreakingBlockPosition() != position) {
            sendCurrentBlockState(owner, position);
            return;
        }

        Level &level = owner.getLevel();
        const BlockState &state = level.getChunk(position.x >> 4, position.z >> 4)
                                        .getBlock(position.x & 15, position.y, position.z & 15);
        if (state.mName == "minecraft:air") {
            stopBreakingBlock(owner, player);
            return;
        }

        const BlockData *blockData = BlockDataTable::find(state.mName.c_str());
        if (!hasReachedBreakTime(owner, player, blockData) ||
            player.getBreakProgress() + BREAK_PROGRESS_EPSILON < 1.0) {
            sendCurrentBlockState(owner, position);
            return;
        }
    }

    if (player.isBreakingBlock())
        stopBreakingBlock(owner, player);

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
    hit.mActorType = "";
    hit.mActorUniqueId = -1;
    broadcastToViewers(owner, center, hit);

    EntityEventPacket swing;
    swing.mRuntimeActorId = player.getRuntimeId();
    swing.mEventId = (uint8_t) EntityEventType::ArmSwing;
    swing.mEventData = 0;
    swing.mHasFirePosition = false;
    broadcastToViewers(owner, center, swing);

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
    if (!player.isSpawned() || player.isDead() || player.getGameType() == (int32_t) GameType::Spectator)
        return;

    if (transaction.mBlockFace < 0 || transaction.mBlockFace >= 6
        || transaction.mHotbarSlot < 0 || transaction.mHotbarSlot >= PlayerInventory::HOTBAR_SIZE)
        return;

    if (transaction.mBlockPosition.y < Chunk::MIN_Y || transaction.mBlockPosition.y > Chunk::MAX_Y)
        return;

    if (!canInteractWithBlock(player, transaction.mBlockPosition))
        return;

    if (!finiteClickPosition(transaction.mClickPosition))
        return;

    PlayerInventory &inventory = player.getInventory();
    if (inventory.getSelectedSlot() != transaction.mHotbarSlot) {
        player.getInventoryManager().onClientSelectHotbarSlot(transaction.mHotbarSlot);
        inventory.setSelectedSlot(transaction.mHotbarSlot);
    }

    const ItemStack &heldItem = inventory.getItemInHand();
    const bool bucket = BucketItem::isBucket(heldItem);
    if (heldItem.isAir() || heldItem.mCount <= 0 || heldItem.mDefinition == nullptr
        || (!bucket && heldItem.mBlockDefinition == nullptr) || transaction.mItemInHand.isAir()
        || transaction.mItemInHand.mCount <= 0 || transaction.mItemInHand.mDefinition == nullptr
        || transaction.mItemInHand.mDefinition->getIdentifier() != heldItem.mDefinition->getIdentifier()
        || transaction.mItemInHand.mDamage != heldItem.mDamage
        || (!bucket && (transaction.mItemInHand.mBlockDefinition == nullptr
                        || transaction.mItemInHand.mBlockDefinition->getIdentifier()
                           != heldItem.mBlockDefinition->getIdentifier()))) {
        owner._sendInventory(player);
        return;
    }

    Level &level = owner.getLevel();
    const BlockState clickedState = level.getBlockState(transaction.mBlockPosition.x,
                                                        transaction.mBlockPosition.y,
                                                        transaction.mBlockPosition.z);

    if (bucket) {
        BucketItem::use(owner, player, transaction);
        return;
    }

    if (clickedState.mName == "minecraft:air") {
        sendCurrentBlockState(owner, transaction.mBlockPosition);
        return;
    }

    if (player.getGameType() == (int32_t) GameType::Adventure
        && std::find(heldItem.mCanPlace.begin(), heldItem.mCanPlace.end(), clickedState.mName)
           == heldItem.mCanPlace.end()) {
        sendCurrentBlockState(owner, transaction.mBlockPosition);
        return;
    }

    broadcastArmSwing(owner, player,
                      Vector3f((float) transaction.mBlockPosition.x + 0.5f,
                               (float) transaction.mBlockPosition.y + 0.5f,
                               (float) transaction.mBlockPosition.z + 0.5f));

    static const int offsets[6][3] = {
            {0,  -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}
    };

    const int face = transaction.mBlockFace;
    Vector3i target = transaction.mBlockPosition;
    if (!isReplaceable(clickedState)) {
        target = Vector3i(transaction.mBlockPosition.x + offsets[face][0],
                          transaction.mBlockPosition.y + offsets[face][1],
                          transaction.mBlockPosition.z + offsets[face][2]);
    }

    if (target.y < Chunk::MIN_Y || target.y > Chunk::MAX_Y) {
        sendCurrentBlockState(owner, target);
        return;
    }

    const BlockState targetState = level.getBlockState(target.x, target.y, target.z);
    if (!isReplaceable(targetState)) {
        sendCurrentBlockState(owner, target);
        return;
    }

    const BlockDefinition &definition = *heldItem.mBlockDefinition;
    const BlockData *placedData = BlockDataTable::find(definition.getIdentifier().c_str());
    if (intersectsPlayer(player, target, placedData)) {
        sendCurrentBlockState(owner, target);
        return;
    }

    const BlockState placedState(definition.getIdentifier(), definition.getState());
    const int32_t blockHash = BlockStateHasher::hash(placedState.mName, placedState.mStates);
    level.setBlockState(target.x, target.y, target.z, placedState);

    if (player.getGameType() != (int32_t) GameType::Creative) {
        ItemStack remaining = heldItem;
        remaining.mCount--;
        if (remaining.mCount <= 0)
            remaining = ItemStack::air();
        inventory.setItemInHand(std::move(remaining));
        player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory,
                                              inventory.getSelectedSlot());
    }

    InventoryHandler::sendHeldItem(owner, player);

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
    placeSound.mActorType = "";
    placeSound.mActorUniqueId = -1;
    broadcastToViewers(owner, targetCenter, placeSound);

}
