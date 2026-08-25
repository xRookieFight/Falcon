#include "Network/Handler/ServerNetworkHandler.h"

#include "Actor/DynamicPropertyStore.h"
#include "Actor/ServerActor.h"
#include "Core/Debug/BedrockLog.h"
#include "Protocol/Packets/AddActorPacket.h"
#include "Protocol/Packets/AnimateEntityPacket.h"
#include "Protocol/Packets/MoveActorAbsolutePacket.h"
#include "Protocol/Packets/PlaySoundPacket.h"
#include "Protocol/Packets/LevelSoundEventPacket.h"
#include "Protocol/Packets/RemoveActorPacket.h"
#include "Protocol/Packets/SetActorDataPacket.h"
#include "Protocol/Packets/SetActorMotionPacket.h"
#include "Protocol/Packets/ActorEventPacket.h"
#include "Protocol/Packets/SpawnParticleEffectPacket.h"
#include "Protocol/Packets/CameraInstructionPacket.h"
#include "Protocol/Packets/MobEffectPacket.h"
#include "Protocol/Packets/PlayerStartItemCooldownPacket.h"
#include "Protocol/Packets/RemoveObjectivePacket.h"
#include "Protocol/Packets/SetDisplayObjectivePacket.h"
#include "Protocol/Packets/SetPlayerGameTypePacket.h"
#include "Protocol/Packets/SetScorePacket.h"
#include "Protocol/Packets/SetTitlePacket.h"
#include "Protocol/Packets/TextPacket.h"
#include "Item/Item.h"
#include "Item/ItemData.h"
#include "Item/ItemEnchantments.h"
#include "Item/EnchantmentData.h"
#include "Item/Items/ThrowableItems.h"
#include "Item/PotionEffects.h"
#include "Item/StringToItemParser.h"

#include <random>
#include "Network/Handler/BlockActionHandler.h"
#include "Network/Handler/ItemActorHandler.h"
#include "Scripting/Content/CustomContentRegistry.h"

#include <algorithm>
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

ServerActor *ServerNetworkHandler::spawnProjectile(ServerPlayer &player, const std::string &identifier, float speed) {
    const float degreesToRadians = 3.14159265358979323846f / 180.0f;
    const Vector3f &rotation = player.getRotation();
    const float pitch = rotation.x * degreesToRadians;
    const float yaw = rotation.y * degreesToRadians;

    const Vector3f direction(-std::sin(yaw) * std::cos(pitch), -std::sin(pitch), std::cos(yaw) * std::cos(pitch));

    Vector3f spawnPosition = player.getPosition();
    spawnPosition.y += 1.5f;

    ServerActor *projectile = spawnActor(identifier, spawnPosition);
    if (projectile == nullptr)
        return nullptr;

    projectile->setProjectile(true);
    projectile->setOwnerUniqueId((int64_t) player.getRuntimeId());
    projectile->setMotion(Vector3f(direction.x * speed, direction.y * speed, direction.z * speed));
    return projectile;
}

bool ServerNetworkHandler::onThrownProjectileHit(ServerActor &projectile, const Vector3f &hitPosition,
                                                 ServerPlayer *hitPlayer) {
    const std::string identifier = projectile.getIdentifier();

    if (identifier == "minecraft:ender_pearl") {
        const int64_t ownerId = projectile.getOwnerUniqueId();
        for (auto &entry: mPlayers) {
            ServerPlayer &shooter = entry.second;
            if ((int64_t) shooter.getRuntimeId() != ownerId)
                continue;

            shooter.teleport(*this, hitPosition, MovePlayerTeleportationCause::Behavior);
            applyDamage(shooter, 5.0f, "death.fell.accident.generic", {shooter.getName()}, false, false);
            break;
        }
        spawnParticleEffect("minecraft:endermanpop_emitter", hitPosition);
        return true;
    }

    if (identifier == "minecraft:snowball") {
        if (hitPlayer != nullptr) {
            const Vector3f target = hitPlayer->getPosition();
            knockBack(*hitPlayer, target.x - hitPosition.x, target.z - hitPosition.z, 0.3f);
        }
        spawnParticleEffect("minecraft:snowballpoof", hitPosition);
        return true;
    }

    if (identifier == "minecraft:egg") {
        static std::mt19937 eggRng(0x51ED270Bu);
        if (eggRng() % 8 == 0)
            spawnActor("minecraft:chicken", hitPosition);
        return true;
    }

    if (identifier == "minecraft:wind_charge_projectile") {
        const float burstRadius = 3.5f;
        for (auto &entry: mPlayers) {
            ServerPlayer &nearby = entry.second;
            if (!nearby.isSpawned())
                continue;

            const Vector3f position = nearby.getPosition();
            const float dx = position.x - hitPosition.x;
            const float dy = position.y - hitPosition.y;
            const float dz = position.z - hitPosition.z;
            const float distanceSquared = dx * dx + dy * dy + dz * dz;
            if (distanceSquared > burstRadius * burstRadius)
                continue;

            const float falloff = 1.0f - std::sqrt(distanceSquared) / burstRadius;
            knockBack(nearby, dx, dz, 1.2f * falloff, 0.9f);
        }

        for (auto &entry: mActors) {
            ServerActor &nearby = *entry.second;
            if (!nearby.isAlive() || nearby.isProjectile())
                continue;

            const Vector3f position = nearby.getPosition();
            const float dx = position.x - hitPosition.x;
            const float dy = position.y - hitPosition.y;
            const float dz = position.z - hitPosition.z;
            const float distanceSquared = dx * dx + dy * dy + dz * dz;
            if (distanceSquared > burstRadius * burstRadius)
                continue;

            const float falloff = 1.0f - std::sqrt(distanceSquared) / burstRadius;
            knockBack(nearby, dx, dz, 1.2f * falloff, 0.9f);
        }

        const Vector3f burstPosition(hitPosition.x, hitPosition.y + 1.0f, hitPosition.z);
        spawnParticleEffect("minecraft:wind_explosion_emitter", burstPosition);
        playLevelSound(LevelSoundEvent::WIND_CHARGE_BURST, burstPosition);
        return true;
    }

    if (identifier == "minecraft:splash_potion") {
        const auto it = mProjectilePotionId.find(projectile.getUniqueId());
        const int32_t potionId = it != mProjectilePotionId.end() ? it->second : 0;
        if (it != mProjectilePotionId.end())
            mProjectilePotionId.erase(it);

        for (auto &entry: mPlayers) {
            ServerPlayer &nearby = entry.second;
            if (!nearby.isSpawned())
                continue;

            const Vector3f position = nearby.getPosition();
            const float dx = position.x - hitPosition.x;
            const float dy = position.y - hitPosition.y;
            const float dz = position.z - hitPosition.z;
            const float distanceSquared = dx * dx + dy * dy + dz * dz;
            if (distanceSquared > 16.0f)
                continue;

            const float scale = std::max(0.25f, 1.0f - std::sqrt(distanceSquared) / 4.0f);
            applyPotionEffects(nearby, potionId, scale);
        }

        spawnParticleEffect("minecraft:splash_spell_emitter", hitPosition);
        return true;
    }

    if (identifier == "minecraft:lingering_potion") {
        const auto it = mProjectilePotionId.find(projectile.getUniqueId());
        const int32_t potionId = it != mProjectilePotionId.end() ? it->second : 0;
        if (it != mProjectilePotionId.end())
            mProjectilePotionId.erase(it);

        ServerActor *cloud = spawnActor("minecraft:area_effect_cloud", hitPosition);
        if (cloud != nullptr) {
            const float cloudRadius = 3.0f;

            EntityDataMap metadata;

            EntityDataEntry radius;
            radius.mId = ACTOR_DATA_AREA_EFFECT_CLOUD_RADIUS;
            radius.mFormat = EntityDataFormat::Float;
            radius.mFloatValue = cloudRadius;
            metadata.mEntries.push_back(radius);

            EntityDataEntry particle;
            particle.mId = ACTOR_DATA_AREA_EFFECT_CLOUD_PARTICLE_ID;
            particle.mFormat = EntityDataFormat::Int;
            particle.mIntValue = AREA_EFFECT_CLOUD_POTION_PARTICLE;
            metadata.mEntries.push_back(particle);

            EntityDataEntry auxValue;
            auxValue.mId = ACTOR_DATA_POTION_AUX_VALUE;
            auxValue.mFormat = EntityDataFormat::Short;
            auxValue.mShortValue = (int16_t) potionId;
            metadata.mEntries.push_back(auxValue);

            EntityDataEntry color;
            color.mId = ACTOR_DATA_POTION_COLOR;
            color.mFormat = EntityDataFormat::Int;
            color.mIntValue = getPotionColor(potionId);
            metadata.mEntries.push_back(color);

            EntityDataEntry width;
            width.mId = ACTOR_DATA_WIDTH;
            width.mFormat = EntityDataFormat::Float;
            width.mFloatValue = cloudRadius;
            metadata.mEntries.push_back(width);

            EntityDataEntry height;
            height.mId = ACTOR_DATA_HEIGHT;
            height.mFormat = EntityDataFormat::Float;
            height.mFloatValue = 0.5f;
            metadata.mEntries.push_back(height);

            sendActorMetadata(*cloud, metadata);

            LingeringCloud state;
            state.mPotionId = potionId;
            state.mTicksRemaining = 600;
            state.mReapplyTimer = 0;
            state.mPosition = hitPosition;
            mLingeringClouds[cloud->getUniqueId()] = state;
        }
        return true;
    }

    return isThrownProjectile(identifier);
}

bool ServerNetworkHandler::onThrownProjectileHitActor(ServerActor &projectile, const Vector3f &hitPosition,
                                                      ServerActor &hitActor) {
    const std::string identifier = projectile.getIdentifier();

    if (hitActor.isProjectile()) {
        const bool handled = onThrownProjectileHit(projectile, hitPosition, nullptr);
        removeActor(hitActor.getUniqueId());
        return handled;
    }

    if (identifier == "minecraft:snowball") {
        damageActor(hitActor, 0.0f, nullptr);
        knockBack(hitActor, hitActor.getPosition().x - hitPosition.x, hitActor.getPosition().z - hitPosition.z, 0.3f);
        spawnParticleEffect("minecraft:snowballpoof", hitPosition);
        return true;
    }

    if (identifier == "minecraft:egg") {
        knockBack(hitActor, hitActor.getPosition().x - hitPosition.x, hitActor.getPosition().z - hitPosition.z, 0.2f);
        return true;
    }

    return onThrownProjectileHit(projectile, hitPosition, nullptr);
}

void ServerNetworkHandler::setProjectilePotionData(int64_t uniqueId, int32_t potionId) {
    mProjectilePotionId[uniqueId] = potionId;
}

void ServerNetworkHandler::applyPotionEffects(ServerPlayer &player, int32_t potionId, float durationScale) {
    for (const PotionEffect &effect: getPotionEffects(potionId)) {
        if (effect.mInstant) {
            if (effect.mId == MobEffectId::InstantHealth) {
                player.heal(4.0f * (float) (1 << effect.mAmplifier));
            } else if (effect.mId == MobEffectId::InstantDamage) {
                applyDamage(player, 6.0f * (float) (1 << effect.mAmplifier), "death.attack.magic",
                            {player.getName()}, false, false);
            }
            continue;
        }

        MobEffectInstance instance;
        instance.mId = effect.mId;
        instance.mDuration = (int32_t) ((float) effect.mDuration * durationScale);
        if (instance.mDuration < 1)
            instance.mDuration = 1;
        instance.mAmplifier = effect.mAmplifier;
        instance.mParticles = true;
        player.addEffect(instance);
    }
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

void ServerNetworkHandler::sendActorMetadata(ServerActor &actor, const EntityDataMap &metadata) {
    SetActorDataPacket packet;
    packet.mRuntimeActorId = (int64_t) actor.getRuntimeId();
    packet.mMetadata = metadata;
    packet.mTick = (int64_t) mCurrentTick;

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
}

void ServerNetworkHandler::sendActorMotion(Actor &actor) {
    SetActorMotionPacket packet;
    packet.mRuntimeActorId = (int64_t) actor.getRuntimeId();
    packet.mMotion = actor.getMotion();
    packet.mTick = (uint64_t) mCurrentTick;

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, packet, mCodecContext);
    }
}

void ServerNetworkHandler::knockBack(Actor &actor, float deltaX, float deltaZ, float force, float verticalLimit) {
    actor.knockBack(deltaX, deltaZ, force, verticalLimit);
    sendActorMotion(actor);
}

bool ServerNetworkHandler::damageActor(ServerActor &actor, float amount, ServerPlayer *source) {
    if (!actor.isAlive() || amount <= 0.0f)
        return false;

    actor.setHealth(actor.getHealth() - amount);

    ActorEventPacket hurt;
    hurt.mRuntimeActorId = actor.getRuntimeId();
    hurt.mEventId = (uint8_t) EntityEventType::HurtAnimation;
    hurt.mEventData = 0;
    hurt.mHasFirePosition = false;

    for (auto &entry: mPlayers) {
        if (entry.second.isSpawned())
            mNetworkHandler->send(entry.first, hurt, mCodecContext);
    }

    playLevelSound(LevelSoundEvent::HIT, actor.getPosition(), actor.getIdentifier());

    if (source != nullptr) {
        const Vector3f actorPosition = actor.getPosition();
        const Vector3f sourcePosition = source->getPosition();
        const float dx = actorPosition.x - sourcePosition.x;
        const float dz = actorPosition.z - sourcePosition.z;
        knockBack(actor, dx, dz, 0.4f);
    }

    if (actor.getHealth() <= 0.0f) {
        ActorEventPacket death;
        death.mRuntimeActorId = actor.getRuntimeId();
        death.mEventId = (uint8_t) EntityEventType::DeathAnimation;
        death.mEventData = 0;
        death.mHasFirePosition = false;

        for (auto &entry: mPlayers) {
            if (entry.second.isSpawned())
                mNetworkHandler->send(entry.first, death, mCodecContext);
        }

        removeActor(actor.getUniqueId());
    }

    return true;
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

void ServerNetworkHandler::playLevelSound(const std::string &sound, const Vector3f &position,
                                          const std::string &actorType, int32_t extraData) {
    LevelSoundEventPacket packet;
    packet.mSound = sound;
    packet.mPosition = position;
    packet.mExtraData = extraData;
    packet.mActorType = actorType;
    packet.mIsBabyMob = false;
    packet.mDisableRelativeVolume = false;
    packet.mActorUniqueId = -1;
    packet.mHasFirePosition = false;

    BlockActionHandler::broadcastToViewers(*this, position, packet);
}

void ServerNetworkHandler::playNamedSound(const std::string &sound, const Vector3f &position, float volume,
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
    info.mType = ScorerType::Invalid;
    info.mName = participant;

    SetScorePacket packet;
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
                                              const std::string &typeId, int32_t amount, int32_t damage,
                                              const Tag &dynamicProperties) {
    ItemStack stack;
    if (!typeId.empty()) {
        Item item;
        if (StringToItemParser::getInstance().parse(typeId, item)) {
            stack.mDefinition = mItemDefinitions.getDefinition(item.getIdentifier());
            stack.mBlockDefinition = mBlockDefinitions.getDefinition(item.getIdentifier());
            stack.mCount = amount < 1 ? 1 : amount;
            stack.mDamage = damage < 0 ? 0 : damage;

            if (!dynamicProperties.isEmpty()) {
                if (!stack.mTag.isCompound())
                    stack.mTag = Tag::ofCompound();
                stack.mTag.put("DynamicProperties", dynamicProperties);
            }
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

void ServerNetworkHandler::damagePlayerHeldItem(ServerPlayer &player, int32_t amount) {
    if (amount <= 0 || player.getGameType() == (int32_t) GameType::Creative)
        return;

    PlayerInventory &inventory = player.getInventory();
    ItemStack held = inventory.getItemInHand();
    if (held.isAir() || held.mDefinition == nullptr)
        return;

    const ItemData *itemData = ItemDataTable::find(held.mDefinition->getIdentifier());
    if (itemData == nullptr || itemData->mMaxDurability <= 0)
        return;

    static std::mt19937 durabilityRng(0x9E3779B9u);
    const int32_t unbreaking = ItemEnchantments::getLevel(held, EnchantmentIds::UNBREAKING);

    int32_t applied = 0;
    for (int32_t i = 0; i < amount; i++) {
        if (unbreaking <= 0 || (durabilityRng() % (uint32_t) (unbreaking + 1)) == 0)
            applied++;
    }

    if (applied == 0)
        return;

    held.mDamage += applied;
    if (held.mDamage >= itemData->mMaxDurability) {
        inventory.setItemInHand(ItemStack::air());
        playLevelSound(LevelSoundEvent::BREAK, player.getPosition(), "minecraft:player");
    } else {
        inventory.setItemInHand(std::move(held));
    }

    player.getInventoryManager().syncSlot(InventoryManager::InventoryId::Inventory, inventory.getSelectedSlot());
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
            const bool thrown = isThrownProjectile(actor.getIdentifier());
            motion.y -= thrown ? 0.03f : ACTOR_GRAVITY;
            if (thrown) {
                motion.x *= 0.99f;
                motion.y *= 0.99f;
                motion.z *= 0.99f;
            }

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
                const Vector3f hitPosition((float) blockX + 0.5f, (float) blockY + 0.5f, (float) blockZ + 0.5f);
                if (!onThrownProjectileHit(actor, hitPosition, nullptr))
                    mScriptEngine.onProjectileHitBlock(actor, blockX, blockY, blockZ);
                expired.push_back(entry.first);
                continue;
            }

            if (actor.getLifetimeTicks() > 1 && isThrownProjectile(actor.getIdentifier())) {
                ServerPlayer *hitPlayer = nullptr;
                for (auto &playerEntry: mPlayers) {
                    ServerPlayer &candidate = playerEntry.second;
                    if (!candidate.isSpawned())
                        continue;
                    if ((int64_t) candidate.getRuntimeId() == actor.getOwnerUniqueId() &&
                        actor.getLifetimeTicks() < 8)
                        continue;

                    const Vector3f candidatePosition = candidate.getPosition();
                    const float dx = candidatePosition.x - position.x;
                    const float dy = (candidatePosition.y + 1.0f) - position.y;
                    const float dz = candidatePosition.z - position.z;
                    if (dx * dx + dy * dy + dz * dz <= 1.1f) {
                        hitPlayer = &candidate;
                        break;
                    }
                }

                if (hitPlayer != nullptr) {
                    onThrownProjectileHit(actor, position, hitPlayer);
                    expired.push_back(entry.first);
                    continue;
                }

                ServerActor *hitActor = nullptr;
                for (auto &actorEntry: mActors) {
                    ServerActor &candidate = *actorEntry.second;
                    if (&candidate == &actor || !candidate.isAlive())
                        continue;

                    const bool candidateIsProjectile = candidate.isProjectile();
                    if (candidateIsProjectile && candidate.getOwnerUniqueId() == actor.getOwnerUniqueId() &&
                        candidate.getLifetimeTicks() < 8)
                        continue;

                    const Vector3f candidatePosition = candidate.getPosition();
                    const float dx = candidatePosition.x - position.x;
                    const float dy = (candidatePosition.y + (candidateIsProjectile ? 0.0f : 0.9f)) - position.y;
                    const float dz = candidatePosition.z - position.z;
                    const float hitRadius = candidateIsProjectile ? 0.36f : 1.1f;
                    if (dx * dx + dy * dy + dz * dz <= hitRadius) {
                        hitActor = &candidate;
                        break;
                    }
                }

                if (hitActor != nullptr) {
                    onThrownProjectileHitActor(actor, position, *hitActor);
                    expired.push_back(entry.first);
                    continue;
                }
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

    std::vector<int64_t> expiredClouds;
    for (auto &entry: mLingeringClouds) {
        LingeringCloud &cloud = entry.second;
        cloud.mTicksRemaining -= 1;
        cloud.mReapplyTimer -= 1;

        if (cloud.mReapplyTimer <= 0) {
            cloud.mReapplyTimer = 20;
            for (auto &playerEntry: mPlayers) {
                ServerPlayer &nearby = playerEntry.second;
                if (!nearby.isSpawned())
                    continue;

                const Vector3f position = nearby.getPosition();
                const float dx = position.x - cloud.mPosition.x;
                const float dy = position.y - cloud.mPosition.y;
                const float dz = position.z - cloud.mPosition.z;
                if (dx * dx + dy * dy + dz * dz <= 9.0f)
                    applyPotionEffects(nearby, cloud.mPotionId, 0.25f);
            }
        }

        if (cloud.mTicksRemaining <= 0)
            expiredClouds.push_back(entry.first);
    }

    for (const int64_t uniqueId: expiredClouds) {
        mLingeringClouds.erase(uniqueId);
        removeActor(uniqueId);
    }

    for (const int64_t uniqueId: expired)
        removeActor(uniqueId);
}
