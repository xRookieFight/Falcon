#include "network/handler/ServerNetworkHandler.h"

#include "actor/DynamicPropertyStore.h"
#include "actor/ServerActor.h"
#include "core/debug/BedrockLog.h"
#include "protocol/packets/AddActorPacket.h"
#include "protocol/packets/AnimateEntityPacket.h"
#include "protocol/packets/MoveActorAbsolutePacket.h"
#include "protocol/packets/PlaySoundPacket.h"
#include "protocol/packets/RemoveActorPacket.h"
#include "protocol/packets/SetActorDataPacket.h"
#include "protocol/packets/SpawnParticleEffectPacket.h"
#include "protocol/packets/CameraInstructionPacket.h"
#include "protocol/packets/MobEffectPacket.h"
#include "protocol/packets/PlayerStartItemCooldownPacket.h"
#include "protocol/packets/RemoveObjectivePacket.h"
#include "protocol/packets/SetDisplayObjectivePacket.h"
#include "protocol/packets/SetPlayerGameTypePacket.h"
#include "protocol/packets/SetScorePacket.h"
#include "protocol/packets/SetTitlePacket.h"
#include "protocol/packets/TextPacket.h"
#include "item/Item.h"
#include "item/StringToItemParser.h"
#include "network/handler/ItemActorHandler.h"
#include "scripting/content/CustomContentRegistry.h"

#include <cmath>

namespace {
    const float ACTOR_GRAVITY = 0.05f;
    const int32_t PROJECTILE_MAX_LIFETIME = 1200;

    EntityProperties buildActorProperties(ServerActor &actor) {
        EntityProperties properties;

        const CustomActorDefinition *definition = actor.getDefinition();
        if (definition == nullptr)
            return properties;

        for (const ActorPropertyDescription &descriptor: definition->mProperties) {
            if (descriptor.mType == ActorPropertyDescription::Type::Float) {
                FloatEntityProperty property;
                property.mIndex = descriptor.mIndex;
                property.mValue = actor.getFloatProperty(descriptor.mName, descriptor.mDefaultFloat);
                properties.mFloatProperties.push_back(property);
            } else {
                IntEntityProperty property;
                property.mIndex = descriptor.mIndex;
                property.mValue = actor.getIntProperty(descriptor.mName, descriptor.mDefaultInt);
                properties.mIntProperties.push_back(property);
            }
        }

        return properties;
    }
}

ServerActor *ServerNetworkHandler::spawnActor(const std::string &identifier, const Vector3f &position) {
    const uint64_t runtimeId = allocateRuntimeId();
    const int64_t uniqueId = (int64_t) runtimeId;

    std::unique_ptr<ServerActor> actor(new ServerActor(runtimeId, identifier));
    actor->setPosition(position);

    const CustomActorDefinition *definition = CustomContentRegistry::getInstance().getActorDefinition(identifier);
    if (definition != nullptr) {
        actor->setDefinition(definition);
        actor->setProjectile(definition->mIsProjectile);

        for (const ActorPropertyDescription &descriptor: definition->mProperties) {
            if (descriptor.mType == ActorPropertyDescription::Type::Float)
                actor->setFloatProperty(descriptor.mName, descriptor.mDefaultFloat);
            else
                actor->setIntProperty(descriptor.mName, descriptor.mDefaultInt);
        }
    }

    ServerActor *result = actor.get();
    mActors[uniqueId] = std::move(actor);

    broadcastActorSpawn(*result);
    return result;
}

ServerActor *ServerNetworkHandler::getActor(int64_t uniqueId) {
    auto it = mActors.find(uniqueId);
    return it == mActors.end() ? nullptr : it->second.get();
}

void ServerNetworkHandler::removeActor(int64_t uniqueId) {
    auto it = mActors.find(uniqueId);
    if (it == mActors.end())
        return;

    broadcastActorRemove(*it->second);
    mActors.erase(it);
}

void ServerNetworkHandler::broadcastActorSpawn(ServerActor &actor) {
    AddActorPacket packet;
    packet.mUniqueActorId = actor.getUniqueId();
    packet.mRuntimeActorId = (int64_t) actor.getRuntimeId();
    packet.mIdentifier = actor.getTypeId();
    packet.mPosition = actor.getPosition();
    packet.mMotion = actor.getMotion();
    packet.mRotation = Vector2f(actor.getRotation().x, actor.getRotation().y);
    packet.mProperties = buildActorProperties(actor);

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
}

void ServerNetworkHandler::sendActorsTo(ServerPlayer &player) {
    for (auto &entry: mActors) {
        ServerActor &actor = *entry.second;
        if (!actor.isAlive())
            continue;

        AddActorPacket packet;
        packet.mUniqueActorId = actor.getUniqueId();
        packet.mRuntimeActorId = (int64_t) actor.getRuntimeId();
        packet.mIdentifier = actor.getTypeId();
        packet.mPosition = actor.getPosition();
        packet.mMotion = actor.getMotion();
        packet.mRotation = Vector2f(actor.getRotation().x, actor.getRotation().y);
        packet.mProperties = buildActorProperties(actor);

        mNetworkHandler->send(player.getNetworkIdentifier(), packet, mCodecContext);
    }
}

void ServerNetworkHandler::broadcastActorRemove(ServerActor &actor) {
    RemoveActorPacket packet;
    packet.mUniqueActorId = actor.getUniqueId();

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
}

void ServerNetworkHandler::syncActorProperties(ServerActor &actor) {
    SetActorDataPacket packet;
    packet.mRuntimeActorId = (int64_t) actor.getRuntimeId();
    packet.mProperties = buildActorProperties(actor);
    packet.mTick = (int64_t) mCurrentTick;

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
}

void ServerNetworkHandler::broadcastActorMove(ServerActor &actor) {
    MoveActorAbsolutePacket move;
    move.mRuntimeActorId = (int64_t) actor.getRuntimeId();
    move.mPosition = actor.getPosition();
    move.mRotation = actor.getRotation();

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, move, mCodecContext);
    }
}

void ServerNetworkHandler::playActorAnimation(ServerActor &actor, const std::string &animation) {
    AnimateEntityPacket packet;
    packet.mAnimation = animation;
    packet.mRuntimeActorIds.push_back(actor.getRuntimeId());

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
}

void ServerNetworkHandler::spawnParticleEffect(const std::string &identifier, const Vector3f &position) {
    SpawnParticleEffectPacket packet;
    packet.mDimensionId = 0;
    packet.mUniqueActorId = -1;
    packet.mPosition = position;
    packet.mIdentifier = identifier;
    packet.mHasMolangVariablesJson = false;

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
}

void ServerNetworkHandler::playLevelSound(const std::string &sound, const Vector3f &position, float volume,
                                          float pitch) {
    PlaySoundPacket packet;
    packet.mSound = sound;
    packet.mPosition = position;
    packet.mVolume = volume;
    packet.mPitch = pitch;

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
}

void ServerNetworkHandler::spawnItemActor(const std::string &typeId, int32_t amount, const Vector3f &position) {
    Item item;
    if (!StringToItemParser::getInstance().parse(typeId, item))
        return;

    std::shared_ptr<ItemDefinition> definition = mItemDefinitions.getDefinition(item.getIdentifier());
    if (definition == nullptr)
        return;

    ItemStack stack;
    stack.mDefinition = definition;
    stack.mBlockDefinition = mBlockDefinitions.getDefinition(item.getIdentifier());
    stack.mCount = amount < 1 ? 1 : amount;

    ItemActorHandler::dropItem(*this, position, stack, Vector3f(0.0f, 0.0f, 0.0f), 10);
}

void ServerNetworkHandler::sendActionBar(ServerPlayer &player, const std::string &text, bool json) {
    SetTitlePacket packet;
    packet.mType = json ? SetTitlePacket::Type::ActionbarJson : SetTitlePacket::Type::Actionbar;
    packet.mText = text;
    mNetworkHandler->send(player.getNetworkIdentifier(), packet, mCodecContext);
}

void ServerNetworkHandler::sendTitle(ServerPlayer &player, const std::string &text, bool json) {
    SetTitlePacket packet;
    packet.mType = json ? SetTitlePacket::Type::TitleJson : SetTitlePacket::Type::Title;
    packet.mText = text;
    mNetworkHandler->send(player.getNetworkIdentifier(), packet, mCodecContext);
}

void ServerNetworkHandler::applyActorEffect(ServerActor &actor, int32_t effectId, int32_t amplifier,
                                            int32_t durationTicks, bool particles) {
    MobEffectPacket packet;
    packet.mRuntimeActorId = actor.getRuntimeId();
    packet.mEvent = MobEffectPacket::Event::Add;
    packet.mEffectId = effectId;
    packet.mAmplifier = amplifier;
    packet.mParticles = particles;
    packet.mDuration = durationTicks;
    packet.mTick = (uint64_t) mCurrentTick;
    packet.mAmbient = false;

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
}

void ServerNetworkHandler::playSoundFor(ServerPlayer &player, const std::string &sound, const Vector3f &position,
                                        float volume, float pitch) {
    PlaySoundPacket packet;
    packet.mSound = sound;
    packet.mPosition = position;
    packet.mVolume = volume;
    packet.mPitch = pitch;
    mNetworkHandler->send(player.getNetworkIdentifier(), packet, mCodecContext);
}

void ServerNetworkHandler::startPlayerItemCooldown(ServerPlayer &player, const std::string &category,
                                                   int32_t durationTicks) {
    PlayerStartItemCooldownPacket packet;
    packet.mItemCategory = category;
    packet.mCooldownDuration = durationTicks;
    mNetworkHandler->send(player.getNetworkIdentifier(), packet, mCodecContext);
}

void ServerNetworkHandler::clearPlayerCamera(ServerPlayer &player) {
    CameraInstructionPacket packet;
    packet.mHasClear = true;
    packet.mClear = true;
    mNetworkHandler->send(player.getNetworkIdentifier(), packet, mCodecContext);
}

void ServerNetworkHandler::syncPlayerAttributes(ServerPlayer &player) {
    _sendAttributes(player);
}

void ServerNetworkHandler::displayScoreboardObjective(const std::string &slot, const std::string &objectiveId,
                                                      const std::string &displayName) {
    SetDisplayObjectivePacket packet;
    packet.mDisplaySlot = slot;
    packet.mObjectiveId = objectiveId;
    packet.mDisplayName = displayName.empty() ? objectiveId : displayName;
    packet.mCriteria = "dummy";
    packet.mSortOrder = 0;

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
}

void ServerNetworkHandler::clearScoreboardDisplay(const std::string &slot) {
    SetDisplayObjectivePacket packet;
    packet.mDisplaySlot = slot;
    packet.mObjectiveId = "";
    packet.mDisplayName = "";
    packet.mCriteria = "dummy";

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
}

void ServerNetworkHandler::setScoreboardScore(const std::string &objectiveId, const std::string &participant,
                                              int32_t score) {
    const std::string key = objectiveId + "\x1f" + participant;
    auto it = mScoreboardIds.find(key);
    if (it == mScoreboardIds.end())
        it = mScoreboardIds.emplace(key, mNextScoreboardId++).first;

    ScoreInfoEntry info;
    info.mScoreboardId = it->second;
    info.mObjectiveId = objectiveId;
    info.mScore = score;
    info.mType = ScorerType::Fake;
    info.mName = participant;

    SetScorePacket packet;
    packet.mAction = SetScorePacket::Action::Change;
    packet.mInfos.push_back(info);

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
}

void ServerNetworkHandler::removeScoreboardScore(const std::string &objectiveId, const std::string &participant) {
    const std::string key = objectiveId + "\x1f" + participant;
    auto it = mScoreboardIds.find(key);
    if (it == mScoreboardIds.end())
        return;

    ScoreInfoEntry info;
    info.mScoreboardId = it->second;
    info.mObjectiveId = objectiveId;
    info.mType = ScorerType::Fake;
    info.mName = participant;

    SetScorePacket packet;
    packet.mAction = SetScorePacket::Action::Remove;
    packet.mInfos.push_back(info);

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
    mScoreboardIds.erase(it);
}

void ServerNetworkHandler::removeScoreboardObjective(const std::string &objectiveId) {
    RemoveObjectivePacket packet;
    packet.mObjectiveId = objectiveId;

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
}

void ServerNetworkHandler::sendJsonMessage(ServerPlayer &player, const std::string &json) {
    TextPacket packet;
    packet.mType = TextPacket::Type::Json;
    packet.mMessage = json;
    mNetworkHandler->send(player.getNetworkIdentifier(), packet, mCodecContext);
}

void ServerNetworkHandler::playPlayerAnimation(ServerPlayer &player, const std::string &animation) {
    AnimateEntityPacket packet;
    packet.mAnimation = animation;
    packet.mRuntimeActorIds.push_back(player.getRuntimeId());

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
}

void ServerNetworkHandler::setPlayerEquipment(ServerPlayer &player, const std::string &slot,
                                              const std::string &typeId, int32_t amount) {
    ItemStack stack;
    if (!typeId.empty()) {
        Item item;
        if (StringToItemParser::getInstance().parse(typeId, item)) {
            stack.mDefinition = mItemDefinitions.getDefinition(item.getIdentifier());
            stack.mBlockDefinition = mBlockDefinitions.getDefinition(item.getIdentifier());
            stack.mCount = amount < 1 ? 1 : amount;
        }
    }

    PlayerInventory &inventory = player.getInventory();
    if (slot == "Head") {
        inventory.setArmor(PlayerInventory::ARMOR_HEAD, stack);
        player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Armor, PlayerInventory::ARMOR_HEAD);
    } else if (slot == "Chest") {
        inventory.setArmor(PlayerInventory::ARMOR_TORSO, stack);
        player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Armor, PlayerInventory::ARMOR_TORSO);
    } else if (slot == "Legs") {
        inventory.setArmor(PlayerInventory::ARMOR_LEGS, stack);
        player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Armor, PlayerInventory::ARMOR_LEGS);
    } else if (slot == "Feet") {
        inventory.setArmor(PlayerInventory::ARMOR_FEET, stack);
        player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Armor, PlayerInventory::ARMOR_FEET);
    } else if (slot == "Offhand") {
        inventory.setOffhand(stack);
        player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Offhand, 0);
    } else {
        inventory.setItemInHand(stack);
        player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory, inventory.getSelectedSlot());
    }
}

void ServerNetworkHandler::loadWorldDynamicProperties() {
    Tag data;
    if (!mPlayerData.loadData("world_dynamic_properties", data))
        return;

    const Tag *properties = data.get("DynamicProperties");
    if (properties != nullptr)
        deserializeDynamicProperties(*properties, mWorldDynamicProperties);
}

void ServerNetworkHandler::saveWorldDynamicProperties() {
    Tag data = Tag::ofCompound();
    data.put("DynamicProperties", serializeDynamicProperties(mWorldDynamicProperties));
    mPlayerData.saveData("world_dynamic_properties", data);
}

void ServerNetworkHandler::setContainerSlot(ServerPlayer &player, int32_t slot, const std::string &typeId,
                                            int32_t amount, const Tag &dynamicProperties) {
    ItemStack stack;
    if (!typeId.empty()) {
        Item item;
        if (StringToItemParser::getInstance().parse(typeId, item)) {
            stack.mDefinition = mItemDefinitions.getDefinition(item.getIdentifier());
            stack.mBlockDefinition = mBlockDefinitions.getDefinition(item.getIdentifier());
            stack.mCount = amount < 1 ? 1 : amount;
            if (!dynamicProperties.isEmpty()) {
                if (!stack.mTag.isCompound())
                    stack.mTag = Tag::ofCompound();
                stack.mTag.put("DynamicProperties", dynamicProperties);
            }
        }
    }

    PlayerInventory &inventory = player.getInventory();
    if (slot < 0 || slot >= PlayerInventory::CONTAINER_SIZE)
        return;

    inventory.setItem(slot, stack);
    player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory, slot);
}

void ServerNetworkHandler::tickActors() {
    std::vector<int64_t> expired;

    for (auto &entry: mActors) {
        ServerActor &actor = *entry.second;
        actor.addLifetimeTick();

        if (actor.isProjectile()) {
            Vector3f motion = actor.getMotion();
            motion.y -= ACTOR_GRAVITY;

            Vector3f position = actor.getPosition();
            position.x += motion.x;
            position.y += motion.y;
            position.z += motion.z;

            actor.setMotion(motion);
            actor.setPosition(position);

            const int32_t blockX = (int32_t) std::floor(position.x);
            const int32_t blockY = (int32_t) std::floor(position.y);
            const int32_t blockZ = (int32_t) std::floor(position.z);

            if (actor.getLifetimeTicks() > 1 && mLevel.isSolidAt(blockX, blockY, blockZ)) {
                mScriptEngine.onProjectileHitBlock(actor, blockX, blockY, blockZ);
                expired.push_back(entry.first);
                continue;
            }

            MoveActorAbsolutePacket move;
            move.mRuntimeActorId = (int64_t) actor.getRuntimeId();
            move.mPosition = position;
            move.mRotation = actor.getRotation();

            for (auto &playerEntry: mPlayers) {
                if (playerEntry.second.isSpawned())
                    mNetworkHandler->send(playerEntry.first, move, mCodecContext);
            }

            if (actor.getLifetimeTicks() > PROJECTILE_MAX_LIFETIME)
                expired.push_back(entry.first);
        }
    }

    for (const int64_t uniqueId: expired)
        removeActor(uniqueId);
}
