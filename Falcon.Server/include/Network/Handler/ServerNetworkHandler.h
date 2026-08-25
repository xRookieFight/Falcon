#pragma once

#include "Level/BiomeRegistry.h"
#include "Level/Level.h"
#include "Network/Handler/NetworkHandler.h"
#include "Network/PingedCompatibleServer.h"
#include "Network/RakNetInstance.h"
#include "Protocol/NetworkPacketHandler.h"
#include "Protocol/PacketCodecContext.h"
#include "Protocol/Types/BlockDefinitionRegistry.h"
#include "Protocol/Types/ItemDefinitionRegistry.h"
#include "Command/CommandMap.h"
#include "Network/PacketSender.h"
#include "Protocol/Types/CommandOriginData.h"
#include "Player/PlayerDataProvider.h"
#include "Server/OpList.h"
#include "Core/Event/EventBus.h"
#include "Scripting/BehaviorPackManager.h"
#include "Scripting/ScriptEngine.h"
#include "Server/Profiler.h"
#include "Server/PropertiesSettings.h"
#include "Server/ResourcePackManager.h"
#include "Actor/ItemActor.h"
#include "Actor/ServerActor.h"
#include "Actor/ServerPlayer.h"
#include "Protocol/Types/ContainerSlotType.h"
#include "Protocol/Types/EntityDataMap.h"
#include "Protocol/Packets/CraftingDataPacket.h"
#include "Protocol/Packets/CreativeContentPacket.h"

#include <memory>
#include <mutex>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <functional>
#include <chrono>
#include <deque>

class ItemUseTransaction;
class CommandOrigin;
class Block;
class CraftingEventPacket;

class ServerNetworkHandler : public NetworkHandler::Listener,
                             public NetworkPacketHandler,
                             public RakPeerHelper::IPSupportInterface,
                             public PacketSender {
public:
    using ModalFormCallback = std::function<void(ServerPlayer &, const std::string &, bool)>;

    ServerNetworkHandler(const std::string &serverName, const std::string &subName, int maxPlayers);

    ~ServerNetworkHandler() override;

    bool startServerListening(const ConnectionDefinition &definition);

    void stopServerListening();

    void tick();

    void setMotd(const std::string &serverName, const std::string &subName);

    void setProtocolVersion(int protocolVersion, const std::string &gameVersion);

    void setProperties(const PropertiesSettings &properties);

    const PropertiesSettings &getProperties() const { return mProperties; }

    int getActivePlayerCount() const { return (int) mNetworkHandler->getConnectionCount(); }

    int64_t getCurrentTick() const { return mCurrentTick; }

    int64_t getUptimeSeconds() const;

    double getTicksPerSecond() const;

    double getMillisecondsPerTick() const;

    Profiler &getProfiler() { return mProfiler; }

    EventBus &getEventBus() { return mEventBus; }

    ScriptEngine &getScriptEngine() { return mScriptEngine; }

    std::string &getCraftingDataBytes() { return mCraftingDataBytes; }

    std::string &getCreativeContentBytes() { return mCreativeContentBytes; }

    std::string &getItemComponentsBytes() { return mItemComponentsBytes; }

    double getPeakMillisecondsPerTick() const;

    int getMaxPlayers() const { return mMaxPlayers; }

    NetworkHandler &getNetworkHandler() { return *mNetworkHandler; }

    Level &getLevel() { return mLevel; }

    std::unordered_map<NetworkIdentifier, ServerPlayer, NetworkIdentifier::Hasher> &getPlayers() { return mPlayers; }

    std::unordered_map<int64_t, std::unique_ptr<ServerActor>> &getActors() { return mActors; }

    ServerActor *spawnActor(const std::string &identifier, const Vector3f &position);

    ServerActor *spawnProjectile(ServerPlayer &player, const std::string &identifier, float speed);

    bool onThrownProjectileHit(ServerActor &projectile, const Vector3f &hitPosition, ServerPlayer *hitPlayer);

    bool onThrownProjectileHitActor(ServerActor &projectile, const Vector3f &hitPosition, ServerActor &hitActor);

    void applyPotionEffects(ServerPlayer &player, int32_t potionId, float durationScale);

    void setProjectilePotionData(int64_t uniqueId, int32_t potionId);

    ServerActor *getActor(int64_t uniqueId);

    void removeActor(int64_t uniqueId);

    void broadcastActorSpawn(ServerActor &actor);

    void sendActorsTo(ServerPlayer &player);

    void broadcastActorRemove(ServerActor &actor);

    void broadcastActorMove(ServerActor &actor);

    void syncActorProperties(ServerActor &actor);

    void sendActorMetadata(ServerActor &actor, const EntityDataMap &metadata);

    bool damageActor(ServerActor &actor, float amount, ServerPlayer *source);

    void sendActorMotion(Actor &actor);

    void knockBack(Actor &actor, float deltaX, float deltaZ, float force, float verticalLimit = 0.4f);

    void playActorAnimation(ServerActor &actor, const std::string &animation);

    void applyActorEffect(ServerActor &actor, int32_t effectId, int32_t amplifier, int32_t durationTicks,
                          bool particles);

    void playSoundFor(ServerPlayer &player, const std::string &sound, const Vector3f &position, float volume,
                      float pitch);

    void spawnParticleEffect(const std::string &identifier, const Vector3f &position);

    void playLevelSound(const std::string &sound, const Vector3f &position,
                        const std::string &actorType = ":", int32_t extraData = -1);

    void playNamedSound(const std::string &sound, const Vector3f &position, float volume, float pitch);

    void spawnItemActor(const std::string &typeId, int32_t amount, const Vector3f &position);

    void sendActionBar(ServerPlayer &player, const std::string &text, bool json);

    void sendTitle(ServerPlayer &player, const std::string &text, bool json);

    void sendJsonMessage(ServerPlayer &player, const std::string &json);

    void playPlayerAnimation(ServerPlayer &player, const std::string &animation);

    void setPlayerEquipment(ServerPlayer &player, const std::string &slot, const std::string &typeId, int32_t amount,
                            int32_t damage = 0, const Tag &dynamicProperties = Tag());

    void damagePlayerHeldItem(ServerPlayer &player, int32_t amount);

    void setContainerSlot(ServerPlayer &player, int32_t slot, const std::string &typeId, int32_t amount,
                          const Tag &dynamicProperties);

    void startPlayerItemCooldown(ServerPlayer &player, const std::string &category, int32_t durationTicks);

    void clearPlayerCamera(ServerPlayer &player);

    void displayScoreboardObjective(const std::string &slot, const std::string &objectiveId,
                                    const std::string &displayName);

    void clearScoreboardDisplay(const std::string &slot);

    void setScoreboardScore(const std::string &objectiveId, const std::string &participant, int32_t score);

    void removeScoreboardScore(const std::string &objectiveId, const std::string &participant);

    void removeScoreboardObjective(const std::string &objectiveId);

    void syncPlayerAttributes(ServerPlayer &player);

    void tickActors();

    void loadActorsForChunk(int32_t chunkX, int32_t chunkZ);

    void saveActorsForChunk(int32_t chunkX, int32_t chunkZ, bool cull);

    void saveAllActors();

    void syncActorPersistence(const std::vector<int64_t> &activeColumns);

    void emitItemUse(ServerPlayer &player);

    std::unordered_map<std::string, DynamicPropertyValue> &getWorldDynamicProperties() {
        return mWorldDynamicProperties;
    }

    void loadWorldDynamicProperties();

    void saveWorldDynamicProperties();

    std::vector<std::unique_ptr<ItemActor>> &getItemEntities() { return mItemEntities; }

    uint64_t allocateRuntimeId() { return mNextRuntimeId++; }

    BlockDefinitionRegistry &getBlockDefinitions() { return mBlockDefinitions; }

    ItemDefinitionRegistry &getItemDefinitions() { return mItemDefinitions; }

    const PacketCodecContext &getCodecContext() const { return mCodecContext; }

    ServerPlayer *getPlayerByName(const std::string &name);

    std::vector<ServerPlayer *> resolveTargets(CommandOrigin &sender, const std::string &selector);

    std::vector<std::string> getPlayerNames() const;

    void setPlayerGameMode(ServerPlayer &player, int gameMode);

    void setPlayerOp(ServerPlayer &player, bool isOp);

    OpList &getOpList() { return mOps; }

    void sendCommandOutput(ServerPlayer &player, const CommandOriginData &origin, const std::string &message);

    void sendCommandOutput(ServerPlayer &player, const CommandOriginData &origin, const std::string &key,
                           const std::vector<std::string> &parameters);

    void broadcastSystemMessage(const std::string &message);

    void broadcastTranslation(const std::string &key, const std::vector<std::string> &parameters = {});

    void broadcastWorldTime();

    ItemActor *dropItem(const Vector3f &position, const ItemStack &item, const Vector3f &motion, int pickupDelay);

    void applyDamage(ServerPlayer &player, float amount, const std::string &deathMessageKey,
                     const std::vector<std::string> &deathMessageParameters = {},
                     bool applyArmor = true, bool respectCooldown = true);

    void killPlayer(ServerPlayer &player, const std::string &deathMessageKey,
                    const std::vector<std::string> &deathMessageParameters = {});

    void _throwItem(ServerPlayer &player, const ItemStack &item);

    void _useHeldItem(ServerPlayer &player);

    void _consumeHeldItem(ServerPlayer &player);

    void _sendInventory(ServerPlayer &player);

    const std::vector<CreativeItemData> &getCreativeItems() { _buildCreativeContent(); return mCreativeItems; }

    const std::vector<ItemStack> &getRecipeOutputs() { _buildCraftingData(); return mRecipeOutputs; }

    const std::vector<uint32_t> &getRecipeSourceIndices() { _buildCraftingData(); return mRecipeSourceIndices; }

    std::vector<CreativeItemData> &getCreativeItemsMutable() { return mCreativeItems; }

    std::vector<ItemStack> &getRecipeOutputsMutable() { return mRecipeOutputs; }

    std::vector<uint32_t> &getRecipeSourceIndicesMutable() { return mRecipeSourceIndices; }

    CraftingDataPacket &getCachedCraftingData() { return mCachedCraftingData; }

    void _loadPlayerData(ServerPlayer &player);

    void _sendHealth(ServerPlayer &player);

    void _sendEntityData(ServerPlayer &player);

    void _sendChunks(ServerPlayer &player);

    void _handleFallDamage(ServerPlayer &player, const Block *supportBlock = nullptr);

    void _disconnect(const NetworkIdentifier &id, const std::string &reason);

    ResourcePackManager &getResourcePacks() { return mResourcePacks; }

    PingedCompatibleServer &getAnnouncement() { return mAnnouncement; }

    CommandMap &getCommands() { return mCommands; }

    BiomeRegistry &getBiomes() { return mBiomes; }

    std::vector<CreativeItemGroup> &getCreativeGroups() { return mCreativeGroups; }

    PlayerDataProvider &getPlayerDataProvider() { return mPlayerData; }

    void _registerVanillaDefinitions();

    void _sendStartGame(ServerPlayer &player);

    void _sendAbilities(ServerPlayer &player);

    void _addToPlayerList(ServerPlayer &player);

    void _removeFromPlayerList(ServerPlayer &player);

    void _sendAttributes(ServerPlayer &player);

    void _checkTerrainReady(ServerPlayer &player);

    void _sendBiomeDefinitions(ServerPlayer &player);

    void _sendItemComponents(ServerPlayer &player);

    void _sendActorIdentifiers(ServerPlayer &player);

    void _sendCraftingData(ServerPlayer &player);

    void _buildCraftingData();

    void _buildCreativeContent();

    void _sendCreativeContent(ServerPlayer &player);

    void _sendAvailableCommands(ServerPlayer &player);

    bool getKeepInventory() const { return mKeepInventory; }

    void setKeepInventory(bool keepInventory) { mKeepInventory = keepInventory; }

    void sendPacketTo(const NetworkIdentifier &id, const Packet &packet) override;

    void queueConsoleCommand(const std::string &commandLine);

    void sendModalForm(ServerPlayer &player, uint32_t formId, const std::string &formData,
                       ModalFormCallback callback = {});

private:
    bool onValidateIncomingConnection(const NetworkIdentifier &id) override;

    void onNewIncomingConnection(const NetworkIdentifier &id) override;

    void onConnectionClosed(const NetworkIdentifier &id, DisconnectFailReason reason,
                            const std::string &message) override;

    void onDataReceived(const NetworkIdentifier &id, const std::string &data) override;

    void handle(const NetworkIdentifier &id, const RequestNetworkSettingsPacket &packet) override;

    void handle(const NetworkIdentifier &id, const LoginPacket &packet) override;

    void handle(const NetworkIdentifier &id, const ResourcePackClientResponsePacket &packet) override;

    void handle(const NetworkIdentifier &id, const ResourcePackChunkRequestPacket &packet) override;

    void handle(const NetworkIdentifier &id, const SetLocalPlayerAsInitializedPacket &packet) override;

    void handle(const NetworkIdentifier &id, const PlayerAuthInputPacket &packet) override;

    void handle(const NetworkIdentifier &id, const CompletedUsingItemPacket &packet) override;

    void handle(const NetworkIdentifier &id, const CommandRequestPacket &packet) override;

    void handle(const NetworkIdentifier &id, const TextPacket &packet) override;

    void handle(const NetworkIdentifier &id, const PacketViolationWarningPacket &packet) override;

    void handle(const NetworkIdentifier &id, const RequestChunkRadiusPacket &packet) override;

    void handle(const NetworkIdentifier &id, const RequestAbilityPacket &packet) override;

    void handle(const NetworkIdentifier &id, const MobEquipmentPacket &packet) override;

    void handle(const NetworkIdentifier &id, const PlayerHotbarPacket &packet) override;

    void handle(const NetworkIdentifier &id, const ItemStackRequestPacket &packet) override;

    void handle(const NetworkIdentifier &id, const CraftingEventPacket &packet) override;

    void handle(const NetworkIdentifier &id, const InventoryTransactionPacket &packet) override;

    void handle(const NetworkIdentifier &id, const ContainerClosePacket &packet) override;

    void handle(const NetworkIdentifier &id, const InteractPacket &packet) override;

    void handle(const NetworkIdentifier &id, const PlayerActionPacket &packet) override;

    void handle(const NetworkIdentifier &id, const RespawnPacket &packet) override;

    void handle(const NetworkIdentifier &id, const SetPlayerGameTypePacket &packet) override;

    void handle(const NetworkIdentifier &id, const BlockActorDataPacket &packet) override;

    void handle(const NetworkIdentifier &id, const BlockPickRequestPacket &packet) override;

    void handle(const NetworkIdentifier &id, const ActorPickRequestPacket &packet) override;

    void handle(const NetworkIdentifier &id, const EmotePacket &packet) override;

    void handle(const NetworkIdentifier &id, const ModalFormResponsePacket &packet) override;

    void handle(const NetworkIdentifier &id, const PlayerSkinPacket &packet) override;

    ServerPlayer *_getPlayer(const NetworkIdentifier &id);

    int _getServerViewDistance() const;

    void _logPackStack() const;

    size_t _getChunkWorkerThreadCount() const;

    void _loadScripts();

    void _validatePackDependencies();

    void _savePlayerData(const ServerPlayer &player);

    void _handleVoidDamage(ServerPlayer &player);

    void _broadcastEntityEvent(const Actor &entity, uint8_t eventId);

    void _broadcastEntityEvent(const Actor &entity, uint8_t eventId, int32_t eventData);

    void _dropInventoryOnDeath(ServerPlayer &player);

    void _respawnPlayer(ServerPlayer &player);

    void onReceiveIPSupport(RakPeerHelper::IPSupport support) override;

    void _updateServerAnnouncement();

    std::unique_ptr<NetworkHandler> mNetworkHandler;
    RakNetInstance *mRakNetInstance;
    BlockDefinitionRegistry mBlockDefinitions;
    ItemDefinitionRegistry mItemDefinitions;
    PacketCodecContext mCodecContext;

    PropertiesSettings mProperties;
    static const int DEFAULT_VIEW_DISTANCE = 4;
    static const int MAX_VIEW_DISTANCE = 12;
    static const unsigned CHUNKS_PER_TICK = 32;
    static const unsigned CHUNK_REQUESTS_PER_TICK = 64;
    static const size_t SPAWN_CHUNK_THRESHOLD = 56;

    Level mLevel;
    BiomeRegistry mBiomes;
    std::vector<CreativeItemGroup> mCreativeGroups;
    std::vector<CreativeItemData> mCreativeItems;
    CraftingDataPacket mCachedCraftingData;
    std::vector<ItemStack> mRecipeOutputs;
    std::vector<uint32_t> mRecipeSourceIndices;
    PlayerDataProvider mPlayerData;
    OpList mOps;
    ResourcePackManager mResourcePacks;
    CommandMap mCommands;
    PingedCompatibleServer mAnnouncement;
    int mMaxPlayers;
    bool mIsListening;

    std::unordered_map<NetworkIdentifier, ServerPlayer, NetworkIdentifier::Hasher> mPlayers;
    std::unordered_map<int64_t, std::unique_ptr<ServerActor>> mActors;

    std::unordered_map<int64_t, int32_t> mProjectilePotionId;

    struct LingeringCloud {
        int32_t mPotionId;
        int32_t mTicksRemaining;
        int32_t mReapplyTimer;
        Vector3f mPosition;
    };

    std::unordered_map<int64_t, LingeringCloud> mLingeringClouds;
    std::unordered_set<int64_t> mActorLoadedChunks;
    std::unordered_map<std::string, DynamicPropertyValue> mWorldDynamicProperties;
    std::unordered_map<std::string, int64_t> mScoreboardIds;
    int64_t mNextScoreboardId = 1;
    std::unordered_map<NetworkIdentifier, std::unordered_map<uint32_t, ModalFormCallback>,
                       NetworkIdentifier::Hasher> mModalFormCallbacks;
    std::vector<std::unique_ptr<ItemActor>> mItemEntities;
    bool mKeepInventory;
    uint64_t mNextRuntimeId;
    int64_t mCurrentTick = 0;
    std::chrono::steady_clock::time_point mStartTime = std::chrono::steady_clock::now();

    Profiler mProfiler;
    EventBus mEventBus;
    ScriptEngine mScriptEngine;
    BehaviorPackManager mBehaviorPacks;

    std::string mCraftingDataBytes;
    std::string mCreativeContentBytes;
    std::string mItemComponentsBytes;

    static const size_t TICK_SAMPLE_COUNT = 100;
    std::deque<std::chrono::steady_clock::time_point> mTickStartSamples;
    std::deque<double> mTickDurationSamples;

    std::mutex mConsoleQueueMutex;
    std::queue<std::string> mConsoleQueue;
};
